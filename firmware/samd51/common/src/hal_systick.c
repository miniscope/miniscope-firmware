#include "hal_systick.h"
#include "sam.h"

static volatile uint32_t s_ticks;

void hal_systick_init(uint32_t cpu_hz)
{
    if (SysTick_Config(cpu_hz / 1000U)) {  /* 1 ms reload value */
        __BKPT(0);  /* reload value exceeds 24-bit SysTick max */
    }
}

uint32_t hal_systick_get_ticks(void)
{
    return s_ticks;
}

void hal_delay_ms(uint32_t ms)
{
    uint32_t start = s_ticks;
    while ((s_ticks - start) < ms) {
        __WFI();
    }
}

void SysTick_Handler(void)
{
    s_ticks++;
}
