#include "fw_config.h"

#ifdef FW_OUTPUT_SPI

#include "spi_output.h"
#include "board.h"
#include "hal_gpio.h"
#include "hal_clock.h"

#ifdef FW_ENABLE_8B10B
#include "encode_8b10b.h"
static encode_8b10b_state_t s_enc_state;
#endif

/*
 * SPI output driver stub.
 *
 * The specific SERCOM instance and pins depend on the board.
 * Full implementation requires SERCOM SPI master configuration
 * with DMA for efficient bulk transfers.
 */

void spi_output_init(void)
{
#ifdef FW_ENABLE_8B10B
    encode_8b10b_init(&s_enc_state);
#endif

    /* TODO: Configure SERCOM SPI master for output */
}

void spi_output_send(const uint8_t *data, uint32_t len)
{
    (void)data;
    (void)len;

    /* TODO: Implement SPI DMA transfer */
}

#endif /* FW_OUTPUT_SPI */
