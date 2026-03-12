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
 * ~10 us per bit at 120 MHz.
 */

#define SPI_BIT_DELAY_CYCLES 300  /* ~2.5 us at 120 MHz */

static void spi_delay(void)
{
    for (volatile int i = 0; i < 75; i++) {
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
    PORT_REGS->GROUP[BOARD_SENSOR_MISO_PORT].PORT_DIRCLR =
        (1u << BOARD_SENSOR_MISO_PIN);
    PORT_REGS->GROUP[BOARD_SENSOR_MISO_PORT].PORT_PINCFG[BOARD_SENSOR_MISO_PIN] |=
        PORT_PINCFG_INEN_Msk;

    spi_clk_low();
    spi_nss_deassert();
}

void sensor_python480_write_reg(uint16_t addr, uint16_t value)
{
    /* Construct 26-bit frame: addr[8:0] + W(0) + data[15:0] */
    uint32_t frame = ((uint32_t)(addr & 0x1FF) << 17)
                   | (0u << 16)  /* Write bit = 0 */
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
    /* Construct 10-bit command: addr[8:0] + R(1) */
    uint32_t cmd = ((uint32_t)(addr & 0x1FF) << 1) | 1u;

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

    /* Read 16-bit data */
    uint16_t value = 0;
    for (int i = 15; i >= 0; i--) {
        spi_delay();
        spi_clk_high();
        spi_delay();
        if (spi_miso_read()) {
            value |= (1u << i);
        }
        spi_clk_low();
    }

    spi_delay();
    spi_nss_deassert();
    spi_delay();

    return value;
}

/*
 * Python480 register addresses (from NOIP1SN0480A datasheet)
 */
#define PY480_REG_CHIP_ID       0
#define PY480_REG_RESET         2
#define PY480_REG_PLL_CTRL      16
#define PY480_REG_PLL_CFG       17
#define PY480_REG_CLK_CTRL      20
#define PY480_REG_AEC_CTRL      160
#define PY480_REG_GAIN          204
#define PY480_REG_EXPO_HI       201
#define PY480_REG_EXPO_LO       202
#define PY480_REG_SOFT_POWERUP  32
#define PY480_REG_IMAGE_CTRL    192
#define PY480_REG_SUB_ENABLE    210

/* Required upload sequences (from original firmware) */
static const struct { uint16_t addr; uint16_t val; } s_init_seq[] = {
    /* Clock management — enable PLL */
    { PY480_REG_CLK_CTRL,  0x0001 },  /* Enable logic clock */
    { PY480_REG_PLL_CTRL,  0x0001 },  /* Enable PLL */
    { PY480_REG_PLL_CFG,   0x2004 },  /* PLL config for 20 MHz input */

    /* Required uploads (from datasheet "Required Register Upload" table) */
    { 2,   0x0000 },  /* Release soft reset */
    { 17,  0x2004 },  /* PLL config */
    { 20,  0x0001 },  /* Clock control */
    { 26,  0x2280 },  /* Reserved */
    { 27,  0x3D2D },  /* Reserved */
    { 32,  0x2004 },  /* Soft power up */
    { 40,  0x0003 },  /* Reserved */
    { 41,  0x085A },  /* Reserved */
    { 42,  0x4110 },  /* Reserved */
    { 43,  0x0008 },  /* Reserved */
    { 48,  0x0007 },  /* Reserved */
    { 64,  0x0001 },  /* Reserved */
    { 65,  0x382B },  /* Reserved */
    { 66,  0x53C8 },  /* Reserved */
    { 67,  0x0665 },  /* Reserved */
    { 68,  0x0085 },  /* Reserved */
    { 69,  0x0888 },  /* Reserved */
    { 72,  0x0010 },  /* Reserved */
    { 128, 0x4714 },  /* Reserved */
    { 171, 0x1002 },  /* Reserved */
    { 175, 0x0080 },  /* Reserved */
    { 176, 0x00E6 },  /* Reserved */
    { 192, 0x0000 },  /* Image control */
    { 194, 0x0A3C },  /* Reserved */
    { 197, 0x0306 },  /* Reserved */
    { 204, 0x01E1 },  /* Default gain */
    { 207, 0x0000 },  /* Reserved */

    /* Subsample enable for 8-bit mode */
    { PY480_REG_SUB_ENABLE, 0x0099 },

    /* Soft power-up sequence */
    { PY480_REG_SOFT_POWERUP, 0x0003 },
    { PY480_REG_SOFT_POWERUP, 0x2003 },
    { PY480_REG_SOFT_POWERUP, 0x2007 },
};

void sensor_python480_init(void)
{
    spi_init_pins();

    /* Wait for sensor power-up */
    hal_delay_ms(100);

    /* Run init sequence */
    for (uint32_t i = 0; i < sizeof(s_init_seq) / sizeof(s_init_seq[0]); i++) {
        sensor_python480_write_reg(s_init_seq[i].addr, s_init_seq[i].val);
        delay_ms_approx(1);
    }

    /* Final power-up delay */
    hal_delay_ms(100);
}

void sensor_python480_set_gain(python480_gain_t gain)
{
    uint16_t val;
    switch (gain) {
    case PYTHON480_GAIN_1X:   val = 0x01E1; break;
    case PYTHON480_GAIN_2X:   val = 0x01E9; break;
    case PYTHON480_GAIN_3P5X: val = 0x01ED; break;
    default:                  val = 0x01E1; break;
    }
    sensor_python480_write_reg(PY480_REG_GAIN, val);
}

void sensor_python480_set_fps(uint8_t fps)
{
    /* FPS is controlled by exposure + blanking. Adjust image timing
     * registers based on target FPS at the configured sensor clock. */
    uint16_t frame_length;
    switch (fps) {
    case 5:  frame_length = 4000; break;
    case 10: frame_length = 2000; break;
    case 15: frame_length = 1333; break;
    case 20: frame_length = 1000; break;
    default: frame_length = 1000; break;
    }
    sensor_python480_write_reg(194, frame_length);
}

void sensor_python480_set_exposure(uint32_t exposure_us)
{
    /* Convert microseconds to sensor clock counts */
    uint32_t counts = (exposure_us * (BOARD_SENSOR_CLK_HZ / 1000000U));
    sensor_python480_write_reg(PY480_REG_EXPO_HI, (uint16_t)(counts >> 16));
    sensor_python480_write_reg(PY480_REG_EXPO_LO, (uint16_t)(counts & 0xFFFF));
}

#endif /* FW_SENSOR_PYTHON480 */
