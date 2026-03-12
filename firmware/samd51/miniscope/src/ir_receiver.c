#include "fw_config.h"

#ifdef FW_ENABLE_IR_RECEIVER

#include "ir_receiver.h"
#include "board.h"
#include "hal_gpio.h"
#include "hal_eic.h"
#include "hal_clock.h"
#include <stddef.h>

static volatile bool s_cmd_pending;
static volatile uint32_t s_last_command;
static ir_callback_t s_callback;

static void ir_eic_callback(uint8_t extint)
{
    (void)extint;

    /*
     * Basic pulse detection — a falling edge on the IR pin indicates
     * a received IR signal. Full NEC protocol decoding would require
     * timing measurements between edges. For now, any edge triggers
     * a command event.
     */
    s_last_command = 1; /* Generic "trigger" command */
    s_cmd_pending = true;

    if (s_callback) {
        s_callback(s_last_command);
    }
}

void ir_receiver_init(void)
{
    /* Route PB22 to EIC (PMUX A) */
    gpio_set_pmux(BOARD_IR_PORT, BOARD_IR_PIN, PORT_PMUX_PMUXE_A_Val);

    /* Enable MCLK for EIC */
    hal_clock_enable_apb(&MCLK_REGS->MCLK_APBAMASK, MCLK_APBAMASK_EIC_Msk);

    /* Route GCLK0 to EIC for filtering */
    hal_clock_enable_gclk_channel(EIC_GCLK_ID, 0);

    /* Configure EXTINT line for IR */
    hal_eic_config_t eic_cfg = {
        .extint   = BOARD_IR_EXTINT,
        .sense    = HAL_EIC_SENSE_FALL,  /* Falling edge = IR pulse start */
        .filter   = true,
        .debounce = true,
    };
    hal_eic_configure_line(&eic_cfg);

    /* Register callback and enable */
    hal_eic_set_callback(BOARD_IR_EXTINT, ir_eic_callback);
    hal_eic_enable_line(BOARD_IR_EXTINT);

    s_cmd_pending = false;
    s_last_command = 0;
    s_callback = NULL;
}

void ir_receiver_set_callback(ir_callback_t cb)
{
    s_callback = cb;
}

bool ir_receiver_has_command(void)
{
    return s_cmd_pending;
}

uint32_t ir_receiver_get_command(void)
{
    s_cmd_pending = false;
    return s_last_command;
}

#endif /* FW_ENABLE_IR_RECEIVER */
