#ifndef BOARD_H
#define BOARD_H

#include "board_common.h"

/*
 * Miniscope Wire-Free V4 — SAMD51J20A
 *
 * Pin assignments from the original Atmel Studio firmware.
 */

/* --- Status LED (active high) --- */
#define BOARD_LED_PORT          1   /* Port B */
#define BOARD_LED_PIN           9   /* PB09 */

/* --- Excitation LED PWM (TC0_WO0) --- */
#define BOARD_EX_LED_PORT       1   /* Port B */
#define BOARD_EX_LED_PIN        30  /* PB30 — TC0/WO[0], PMUX E */
#define BOARD_EX_LED_EN_PORT    1   /* Port B */
#define BOARD_EX_LED_EN_PIN     1   /* PB01 — enable line */

/* --- Python480 bit-bang SPI --- */
#define BOARD_SENSOR_MOSI_PORT  0   /* Port A */
#define BOARD_SENSOR_MOSI_PIN   4   /* PA04 */
#define BOARD_SENSOR_SCK_PORT   0   /* Port A */
#define BOARD_SENSOR_SCK_PIN    5   /* PA05 */
#define BOARD_SENSOR_MISO_PORT  0   /* Port A */
#define BOARD_SENSOR_MISO_PIN   6   /* PA06 */
#define BOARD_SENSOR_NSS_PORT   1   /* Port B */
#define BOARD_SENSOR_NSS_PIN    12  /* PB12 */

/* --- PCC (Parallel Capture Controller) --- */
#define BOARD_PCC_D0_PORT       0   /* Port A */
#define BOARD_PCC_D0_PIN        16  /* PA16 — PCC/DATA[0], PMUX K */
/* D1-D7: PA17-PA23 (contiguous) */
#define BOARD_PCC_DEN1_PORT     0   /* Port A */
#define BOARD_PCC_DEN1_PIN      12  /* PA12 — PCC/DEN1 (Frame Valid) */
#define BOARD_PCC_DEN2_PORT     0   /* Port A */
#define BOARD_PCC_DEN2_PIN      13  /* PA13 — PCC/DEN2 (Horiz Valid) */
#define BOARD_PCC_CLK_PORT      0   /* Port A */
#define BOARD_PCC_CLK_PIN       14  /* PA14 — PCC/CLK (pixel clock) */

/* --- SDHC0 --- */
#define BOARD_SD_CMD_PORT       0   /* Port A */
#define BOARD_SD_CMD_PIN        8   /* PA08 — SDHC0/CMD, PMUX I */
#define BOARD_SD_DAT0_PORT      0   /* Port A */
#define BOARD_SD_DAT0_PIN       9   /* PA09 — SDHC0/DAT[0] */
/* DAT1: PA10, DAT2: PA11 */
#define BOARD_SD_DAT3_PORT      1   /* Port B */
#define BOARD_SD_DAT3_PIN       10  /* PB10 — SDHC0/DAT[3] */
#define BOARD_SD_CLK_PORT       1   /* Port B */
#define BOARD_SD_CLK_PIN        11  /* PB11 — SDHC0/SDCK */

/* --- I2C bit-bang (EWL lens) --- */
#define BOARD_I2C_SDA_PORT      1   /* Port B */
#define BOARD_I2C_SDA_PIN       0   /* PB00 */
#define BOARD_I2C_SCL_PORT      1   /* Port B */
#define BOARD_I2C_SCL_PIN       2   /* PB02 */

/* --- ADC battery monitor --- */
#define BOARD_ADC_BATT_PORT     0   /* Port A */
#define BOARD_ADC_BATT_PIN      2   /* PA02 — ADC0/AIN[0] */
#define BOARD_ADC_BATT_AIN      0   /* AIN0 */

/* --- External interrupts (EIC) --- */
#define BOARD_IR_PORT           1   /* Port B */
#define BOARD_IR_PIN            22  /* PB22 — EXTINT[6] */
#define BOARD_IR_EXTINT         6

#define BOARD_NCHRG_PORT        1   /* Port B */
#define BOARD_NCHRG_PIN         23  /* PB23 — EXTINT[7] */
#define BOARD_NCHRG_EXTINT      7

#define BOARD_FV_PORT           1   /* Port B */
#define BOARD_FV_PIN            14  /* PB14 — EXTINT[14] */
#define BOARD_FV_EXTINT         14

#define BOARD_BUTTON_PORT       0   /* Port A */
#define BOARD_BUTTON_PIN        25  /* PA25 — EXTINT[9] (odd) */
#define BOARD_BUTTON_EXTINT     9

/* --- Sensor reset --- */
#define BOARD_SENSOR_RESET_PORT 1   /* Port B */
#define BOARD_SENSOR_RESET_PIN  13  /* PB13 */

/* --- SD card detect --- */
#define BOARD_SD_CD_PORT        1   /* Port B */
#define BOARD_SD_CD_PIN         5   /* PB05 */

/* --- UART (debug) --- */
#define BOARD_UART_TX_PORT      1   /* Port B */
#define BOARD_UART_TX_PIN       16  /* PB16 */
#define BOARD_UART_RX_PORT      1   /* Port B */
#define BOARD_UART_RX_PIN       17  /* PB17 */

/* --- Sensor clock output (GCLK1 on PB15) --- */
#define BOARD_SCLK_PORT         1   /* Port B */
#define BOARD_SCLK_PIN          15  /* PB15 — GCLK_IO[1], PMUX M */

/* --- Power control --- */
#define BOARD_EN_3V3_PORT       0   /* Port A */
#define BOARD_EN_3V3_PIN        24  /* PA24 */

/* --- EWL I2C address --- */
#define BOARD_EWL_I2C_ADDR      0x23

/* --- Sensor clock frequency --- */
#define BOARD_SENSOR_CLK_HZ     20000000U   /* 20 MHz */

#endif /* BOARD_H */
