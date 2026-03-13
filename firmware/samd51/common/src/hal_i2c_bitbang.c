#include "hal_i2c_bitbang.h"
#include "hal_gpio.h"
#include "hal_delay.h"

/*
 * Bit-bang I2C at ~100 kHz (standard mode).
 * Half-period delay ~5 us. At 120 MHz, delay_us_approx(5) ≈ 600 cycles.
 */
#define I2C_DELAY_US  5

static void i2c_delay(void)
{
    /* ~5 us at 120 MHz: 600 NOPs */
    for (volatile int i = 0; i < 150; i++) {
        __asm volatile("nop");
    }
}

/* Open-drain helpers: OUT register is always 0.
 * Drive low = set as output (drives 0).
 * Release high = set as input (pull-up pulls high). */

static void sda_low(const hal_i2c_bb_t *bus)
{
    gpio_set_output(bus->sda_port, bus->sda_pin);
}

static void sda_release(const hal_i2c_bb_t *bus)
{
    /* Set as input to release line (external pull-up) */
    PORT_REGS->GROUP[bus->sda_port].PORT_DIRCLR = (1u << bus->sda_pin);
    PORT_REGS->GROUP[bus->sda_port].PORT_PINCFG[bus->sda_pin] |= PORT_PINCFG_INEN_Msk;
}

static void scl_low(const hal_i2c_bb_t *bus)
{
    gpio_set_output(bus->scl_port, bus->scl_pin);
}

static void scl_release(const hal_i2c_bb_t *bus)
{
    PORT_REGS->GROUP[bus->scl_port].PORT_DIRCLR = (1u << bus->scl_pin);
    PORT_REGS->GROUP[bus->scl_port].PORT_PINCFG[bus->scl_pin] |= PORT_PINCFG_INEN_Msk;
}

static uint8_t sda_read(const hal_i2c_bb_t *bus)
{
    return gpio_read(bus->sda_port, bus->sda_pin);
}

static void i2c_start(const hal_i2c_bb_t *bus)
{
    /* SDA goes low while SCL is high */
    sda_low(bus);
    i2c_delay();
    scl_low(bus);
    i2c_delay();
}

static void i2c_stop(const hal_i2c_bb_t *bus)
{
    sda_low(bus);
    i2c_delay();
    scl_release(bus);
    i2c_delay();
    sda_release(bus);
    i2c_delay();
}

static bool i2c_write_byte(const hal_i2c_bb_t *bus, uint8_t byte)
{
    for (int i = 7; i >= 0; i--) {
        if (byte & (1u << i)) {
            sda_release(bus);
        } else {
            sda_low(bus);
        }
        i2c_delay();
        scl_release(bus);
        i2c_delay();
        scl_low(bus);
    }

    /* Read ACK (9th clock) */
    sda_release(bus);
    i2c_delay();
    scl_release(bus);
    i2c_delay();
    uint8_t ack = sda_read(bus);
    scl_low(bus);
    i2c_delay();

    return (ack == 0); /* ACK = SDA low */
}

static uint8_t i2c_read_byte(const hal_i2c_bb_t *bus, bool send_ack)
{
    uint8_t byte = 0;
    sda_release(bus);

    for (int i = 7; i >= 0; i--) {
        i2c_delay();
        scl_release(bus);
        i2c_delay();
        if (sda_read(bus)) {
            byte |= (1u << i);
        }
        scl_low(bus);
    }

    /* Send ACK/NACK */
    if (send_ack) {
        sda_low(bus);
    } else {
        sda_release(bus);
    }
    i2c_delay();
    scl_release(bus);
    i2c_delay();
    scl_low(bus);
    sda_release(bus);
    i2c_delay();

    return byte;
}

void hal_i2c_bb_init(const hal_i2c_bb_t *bus)
{
    /* Set output registers to 0 (open-drain: output=low, input=release) */
    gpio_clear(bus->sda_port, bus->sda_pin);
    gpio_clear(bus->scl_port, bus->scl_pin);

    /* Release both lines (set as input with pull-up enabled) */
    sda_release(bus);
    scl_release(bus);
    i2c_delay();
}

bool hal_i2c_bb_write(const hal_i2c_bb_t *bus,
                      uint8_t addr,
                      const uint8_t *data,
                      uint8_t len)
{
    i2c_start(bus);

    /* Address byte: 7-bit addr + W(0) */
    if (!i2c_write_byte(bus, (uint8_t)(addr << 1))) {
        i2c_stop(bus);
        return false;
    }

    for (uint8_t i = 0; i < len; i++) {
        if (!i2c_write_byte(bus, data[i])) {
            i2c_stop(bus);
            return false;
        }
    }

    i2c_stop(bus);
    return true;
}

bool hal_i2c_bb_read(const hal_i2c_bb_t *bus,
                     uint8_t addr,
                     uint8_t *data,
                     uint8_t len)
{
    i2c_start(bus);

    /* Address byte: 7-bit addr + R(1) */
    if (!i2c_write_byte(bus, (uint8_t)((addr << 1) | 1))) {
        i2c_stop(bus);
        return false;
    }

    for (uint8_t i = 0; i < len; i++) {
        bool ack = (i < (len - 1)); /* ACK all except last byte */
        data[i] = i2c_read_byte(bus, ack);
    }

    i2c_stop(bus);
    return true;
}
