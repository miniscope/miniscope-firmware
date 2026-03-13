#include "fw_config.h"

#ifdef FW_SENSOR_PYTHON480

#include "sensor_python480.h"
#include "board.h"
#include "hal_gpio.h"
#include "hal_delay.h"
#include "hal_systick.h"

/*
 * Python480 bit-bang SPI interface.
 *
 * Protocol: 9-bit address (MSB first) + R/W bit + 16-bit data
 * Clock idle low, data sampled on rising edge.
 * R/W polarity: write=1, read=0 (matches reference python480.h).
 * ~10 us per bit at 60 MHz.
 */

static void spi_delay(void)
{
    /* ~10 us at 60 MHz, matching reference delay_us(10) */
    for (volatile int i = 0; i < 150; i++) {
        __asm volatile("nop");
    }
}

static void spi_clk_low(void)
{
    gpio_clear(BOARD_SENSOR_SCK_PORT, BOARD_SENSOR_SCK_PIN);
}

static void spi_clk_high(void)
{
    gpio_set(BOARD_SENSOR_SCK_PORT, BOARD_SENSOR_SCK_PIN);
}

static void spi_mosi_set(uint8_t val)
{
    if (val) {
        gpio_set(BOARD_SENSOR_MOSI_PORT, BOARD_SENSOR_MOSI_PIN);
    } else {
        gpio_clear(BOARD_SENSOR_MOSI_PORT, BOARD_SENSOR_MOSI_PIN);
    }
}

static uint8_t spi_miso_read(void)
{
    return gpio_read(BOARD_SENSOR_MISO_PORT, BOARD_SENSOR_MISO_PIN);
}

static void spi_nss_assert(void)
{
    gpio_clear(BOARD_SENSOR_NSS_PORT, BOARD_SENSOR_NSS_PIN);
}

static void spi_nss_deassert(void)
{
    gpio_set(BOARD_SENSOR_NSS_PORT, BOARD_SENSOR_NSS_PIN);
}

static void spi_init_pins(void)
{
    gpio_set_output(BOARD_SENSOR_MOSI_PORT, BOARD_SENSOR_MOSI_PIN);
    gpio_set_output(BOARD_SENSOR_SCK_PORT, BOARD_SENSOR_SCK_PIN);
    gpio_set_output(BOARD_SENSOR_NSS_PORT, BOARD_SENSOR_NSS_PIN);

    /* MISO is input — enable input buffer */
    gpio_set_input(BOARD_SENSOR_MISO_PORT, BOARD_SENSOR_MISO_PIN);

    spi_clk_low();
    spi_nss_deassert();
}

void sensor_python480_write_reg(uint16_t addr, uint16_t value)
{
    /* Construct 26-bit frame: addr[8:0] + W(1) + data[15:0] */
    uint32_t frame = ((uint32_t)(addr & 0x1FF) << 17)
                   | (1u << 16)  /* Write bit = 1 */
                   | value;

    spi_nss_assert();
    spi_delay();

    /* Send 26 bits MSB first */
    for (int i = 25; i >= 0; i--) {
        spi_mosi_set((frame >> i) & 1);
        spi_delay();
        spi_clk_high();
        spi_delay();
        spi_clk_low();
    }

    spi_delay();
    spi_nss_deassert();
    spi_delay();
}

uint16_t sensor_python480_read_reg(uint16_t addr)
{
    /* Construct 10-bit command: addr[8:0] + R(0) */
    uint32_t cmd = ((uint32_t)(addr & 0x1FF) << 1) | 0u;

    spi_nss_assert();
    spi_delay();

    /* Send 10-bit address + R */
    for (int i = 9; i >= 0; i--) {
        spi_mosi_set((cmd >> i) & 1);
        spi_delay();
        spi_clk_high();
        spi_delay();
        spi_clk_low();
    }

    /* Extra clock transition between address and data (ref lines 150-151) */
    spi_delay();
    spi_clk_low();
    spi_delay();

    /* Read 16-bit data: SCK high, delay, SCK low, sample MISO */
    uint16_t value = 0;
    for (int i = 15; i >= 0; i--) {
        spi_clk_high();
        spi_delay();
        spi_clk_low();
        spi_delay();
        if (spi_miso_read()) {
            value |= (1u << i);
        }
    }

    spi_delay();
    spi_nss_deassert();
    spi_delay();

    return value;
}

/*
 * Register init tables matching the reference firmware exactly.
 * Sequence: EnableClockMngmnt1 → delay → EnableClockMngmnt2 →
 *           RequiredUploads → SoftPowerUp
 *
 * All values use DISABLE_PLL path (bit 3 set in reg 32).
 */

typedef struct { uint16_t addr; uint16_t val; } reg_pair_t;

/* EnableClockMngmnt1 (8 writes, then 10ms delay) */
static const reg_pair_t s_clk_mgmt1[] = {
    {  2, 0x0000 },
    { 17, 0x2113 },
    { 20, 0x0000 },
    { 26, 0x2280 },
    { 27, 0x3D2D },
    { 32, 0x701C },
    {  8, 0x0000 },
    { 16, 0x0007 },
};

/* EnableClockMngmnt2 (3 writes) */
static const reg_pair_t s_clk_mgmt2[] = {
    {  9, 0x0000 },
    { 32, 0x700E },
    { 34, 0x0001 },
};

/* RequiredUploads — general registers */
static const reg_pair_t s_required_uploads[] = {
    {  2, 0x0000 }, {  8, 0x0000 }, {  9, 0x0000 }, { 10, 0x0000 },
    { 20, 0x0000 }, { 26, 0x2280 }, { 27, 0x3D2D }, { 32, 0x700F },
    { 34, 0x0001 }, { 40, 0x0007 }, { 41, 0x085F }, { 42, 0x4103 },
    { 43, 0x0518 }, { 48, 0x0001 }, { 64, 0x0001 }, { 65, 0x382A },
    { 66, 0x53C8 }, { 67, 0x0665 }, { 68, 0x0085 }, { 69, 0x0888 },
    { 70, 0x4800 }, { 71, 0x8888 }, { 72, 0x0117 }, {112, 0x0000 },
    {128, 0x470A }, {129, 0x8001 }, {130, 0x0015 }, {192, 0x0801 },
    {194, 0x00E4 }, {197, 0x0104 }, {199,     50 }, {200,   3300 },
    {201,   3300 }, {204, 0x00E4 }, {207, 0x0014 }, {214, 0x0100 },
    {215, 0x101F }, {216, 0x0000 }, {219, 0x0023 }, {220, 0x3C2B },
    {221, 0x2B4D }, {224, 0x3E5E }, {211, 0x0049 }, {216, 0x0000 },
    {219, 0x0023 }, {220, 0x3C2B }, {221, 0x2B4D }, {230, 0x0299 },
    {231, 0x0350 }, {232, 0x01F4 }, {235, 0x00E1 }, {256, 0xB019 },
    {258, 0xB019 },
};

/* RequiredUploads — program space (regs 384-476) */
static const reg_pair_t s_program_space[] = {
    {384, 0xC800 }, {385, 0xFB1F }, {386, 0xFB1F }, {387, 0xFB12 },
    {388, 0xF903 }, {389, 0xF802 }, {390, 0xF30F }, {391, 0xF30F },
    {392, 0xF30F }, {393, 0xF30A }, {394, 0xF101 }, {395, 0xF00A },
    {396, 0xF24B }, {397, 0xF226 }, {398, 0xF001 }, {399, 0xF402 },
    {400, 0xF001 }, {401, 0xF402 }, {402, 0xF001 }, {403, 0xF401 },
    {404, 0xF007 }, {405, 0xF20F }, {406, 0xF20F }, {407, 0xF202 },
    {408, 0xF006 }, {409, 0xEC02 }, {410, 0xE801 },
    {419, 0x0030 }, {420, 0x0030 }, {421, 0x0030 }, {422, 0x0030 },
    {423, 0x0030 }, {424, 0x0030 }, {425, 0x0030 }, {426, 0x0030 },
    {427, 0x0030 }, {428, 0x0030 }, {429, 0x0030 }, {430, 0x0030 },
    {431, 0x0030 }, {432, 0x0030 }, {433, 0x0030 }, {434, 0x0030 },
    {435, 0x0030 }, {436, 0x0030 }, {437, 0x0030 }, {438, 0x0030 },
    {439, 0x0030 }, {440, 0x0030 }, {441, 0x0030 }, {442, 0x0030 },
    {443, 0x0030 }, {444, 0x0030 }, {445, 0x0030 }, {446, 0x0030 },
    {447, 0x0030 }, {448, 0x0030 }, {449, 0x0030 }, {450, 0x0030 },
    {451, 0x0030 }, {452, 0x0030 }, {453, 0x0030 }, {454, 0x0030 },
    {455, 0x0030 }, {456, 0x0030 }, {457, 0x0030 }, {458, 0x0030 },
    {459, 0x0030 }, {460, 0x0030 }, {461, 0x0030 }, {462, 0x0030 },
    {463, 0x0030 }, {464, 0x0030 }, {465, 0x0030 }, {466, 0x0030 },
    {467, 0x0030 }, {468, 0x0030 }, {469, 0x0030 }, {470, 0x0030 },
    {471, 0x0030 }, {472, 0x0030 }, {473, 0x0030 }, {474, 0x0030 },
    {475, 0x0030 }, {476, 0x0030 },
};

/* SoftPowerUp (8 writes) */
static const reg_pair_t s_soft_powerup[] = {
    { 10, 0x0000 },
    { 32, 0x700F },
    { 40, 0x0007 },
    { 42, 0x4113 },
    { 48, 0x0001 },
    { 64, 0x0001 },
    { 72, 0x0127 },
    {112, 0x0000 },
};

static void write_reg_table(const reg_pair_t *tbl, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        sensor_python480_write_reg(tbl[i].addr, tbl[i].val);
    }
}

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

void sensor_python480_init(void)
{
    /* Reset sensor via RESET_CMOS pin (PB13) */
    gpio_set_output(BOARD_SENSOR_RESET_PORT, BOARD_SENSOR_RESET_PIN);
    gpio_clear(BOARD_SENSOR_RESET_PORT, BOARD_SENSOR_RESET_PIN);
    hal_delay_ms(100);
    gpio_set(BOARD_SENSOR_RESET_PORT, BOARD_SENSOR_RESET_PIN);
    delay_us_approx(100);

    spi_init_pins();

    /* Wait for sensor power-up */
    hal_delay_ms(100);

    /* EnableClockMngmnt1 */
    write_reg_table(s_clk_mgmt1, ARRAY_LEN(s_clk_mgmt1));
    hal_delay_ms(10);

    /* EnableClockMngmnt2 */
    write_reg_table(s_clk_mgmt2, ARRAY_LEN(s_clk_mgmt2));

    /* RequiredUploads — general registers */
    write_reg_table(s_required_uploads, ARRAY_LEN(s_required_uploads));

    /* RequiredUploads — program space */
    write_reg_table(s_program_space, ARRAY_LEN(s_program_space));

    /* SoftPowerUp */
    write_reg_table(s_soft_powerup, ARRAY_LEN(s_soft_powerup));

    /* Final power-up delay */
    hal_delay_ms(100);
}

void sensor_python480_set_gain(python480_gain_t gain)
{
    uint16_t val;
    switch (gain) {
    case PYTHON480_GAIN_1X:   val = 0x00E1; break;
    case PYTHON480_GAIN_2X:   val = 0x00E4; break;
    case PYTHON480_GAIN_3P5X: val = 0x0024; break;
    default:                  val = 0x00E1; break;
    }
    sensor_python480_write_reg(204, val);
}

void sensor_python480_set_fps(uint8_t fps)
{
    /* FPS controlled via register 201 (0xC9) — frame time in line periods */
    uint16_t frame_length;
    switch (fps) {
    case 5:  frame_length = 20000; break;
    case 10: frame_length = 10000; break;
    case 15: frame_length = 6667;  break;
    case 20: frame_length = 5000;  break;
    default: frame_length = 5000;  break;
    }
    sensor_python480_write_reg(201, frame_length);
}

void sensor_python480_set_exposure(uint32_t exposure_us)
{
    /* Convert microseconds to sensor clock counts */
    uint32_t counts = (exposure_us * (BOARD_SENSOR_CLK_HZ / 1000000U));
    sensor_python480_write_reg(201, (uint16_t)(counts >> 16));
    sensor_python480_write_reg(202, (uint16_t)(counts & 0xFFFF));
}

void sensor_python480_enable_subsample(void)
{
    /* Enable 2x2 subsampling (matching reference Enable_Subsample) */
    sensor_python480_write_reg(192, 0x0883);  /* 0x0803 | 0x0080 */
    sensor_python480_write_reg(194, 0x0FE4);  /* 0x03E4 | 0x0C00 */
}

#endif /* FW_SENSOR_PYTHON480 */
