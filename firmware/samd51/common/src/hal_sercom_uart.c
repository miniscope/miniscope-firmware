#include "hal_sercom_uart.h"

#define UART_SYNC_TIMEOUT 10000U

static void uart_sync_wait(sercom_registers_t *sercom, uint32_t mask)
{
    uint32_t timeout = UART_SYNC_TIMEOUT;
    while ((sercom->USART_INT.SERCOM_SYNCBUSY & mask) && --timeout) {
        /* wait */
    }
    if (!timeout) {
        __BKPT(0);
    }
}

void hal_uart_init(sercom_registers_t *sercom,
                   uint32_t baud_hz,
                   uint32_t gclk_hz,
                   uint8_t txpad)
{
    /* Software reset */
    sercom->USART_INT.SERCOM_CTRLA = SERCOM_USART_INT_CTRLA_SWRST_Msk;
    uart_sync_wait(sercom, SERCOM_USART_INT_SYNCBUSY_SWRST_Msk);

    /*
     * Asynchronous UART mode, internal clock, TX only.
     * BAUD = 65536 * (1 - 16 * baud_hz / gclk_hz)  (arithmetic baud)
     */
    uint64_t baud_val = 65536ULL - ((65536ULL * 16ULL * baud_hz) / gclk_hz);

    sercom->USART_INT.SERCOM_CTRLA =
        SERCOM_USART_INT_CTRLA_MODE_USART_INT_CLK
        | SERCOM_USART_INT_CTRLA_TXPO(txpad)
        | SERCOM_USART_INT_CTRLA_DORD_Msk;  /* LSB first */

    sercom->USART_INT.SERCOM_CTRLB =
        SERCOM_USART_INT_CTRLB_TXEN_Msk;    /* TX enable */
    uart_sync_wait(sercom, SERCOM_USART_INT_SYNCBUSY_CTRLB_Msk);

    sercom->USART_INT.SERCOM_BAUD = (uint16_t)baud_val;

    /* Enable */
    sercom->USART_INT.SERCOM_CTRLA |= SERCOM_USART_INT_CTRLA_ENABLE_Msk;
    uart_sync_wait(sercom, SERCOM_USART_INT_SYNCBUSY_ENABLE_Msk);
}

void hal_uart_putc(sercom_registers_t *sercom, char c)
{
    while (!(sercom->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_DRE_Msk)) {
        /* Wait for data register empty */
    }
    sercom->USART_INT.SERCOM_DATA = (uint16_t)(uint8_t)c;
}

void hal_uart_puts(sercom_registers_t *sercom, const char *str)
{
    while (*str) {
        hal_uart_putc(sercom, *str++);
    }
}

void hal_uart_write(sercom_registers_t *sercom,
                    const uint8_t *data,
                    uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        hal_uart_putc(sercom, (char)data[i]);
    }
}
