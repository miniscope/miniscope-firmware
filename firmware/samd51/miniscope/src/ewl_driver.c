#include "fw_config.h"

#ifdef FW_ENABLE_EWL

#include "ewl_driver.h"
#include "board.h"
#include "hal_i2c_bitbang.h"
#include "hal_systick.h"

static hal_i2c_bb_t s_bus;

void ewl_init(void)
{
    s_bus.sda_port = BOARD_I2C_SDA_PORT;
    s_bus.sda_pin  = BOARD_I2C_SDA_PIN;
    s_bus.scl_port = BOARD_I2C_SCL_PORT;
    s_bus.scl_pin  = BOARD_I2C_SCL_PIN;

    hal_i2c_bb_init(&s_bus);
}

void ewl_set_voltage(uint8_t voltage)
{
    uint8_t data[1] = { voltage };
    hal_i2c_bb_write(&s_bus, BOARD_EWL_I2C_ADDR, data, 1);
}

void ewl_scan(uint8_t start, uint8_t stop, uint8_t step,
              uint32_t interval_ms)
{
    if (step == 0) return;

    if (start <= stop) {
        for (uint16_t v = start; v <= stop; v += step) {
            ewl_set_voltage((uint8_t)v);
            hal_delay_ms(interval_ms);
            if (v + step < v) break; /* Overflow protection */
        }
    } else {
        for (int16_t v = start; v >= (int16_t)stop; v -= step) {
            ewl_set_voltage((uint8_t)v);
            hal_delay_ms(interval_ms);
        }
    }
}

#endif /* FW_ENABLE_EWL */
