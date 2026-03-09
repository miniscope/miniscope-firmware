#include "sam.h"
#include "board.h"
#include "hal_systick.h"
#include "hal_cycle_count.h"
#include "app.h"

int main(void)
{
    /* SystemInit() is called by the DFP startup code before main */
    board_init();
    hal_systick_init(48000000UL);   /* DFLL default boot clock */
    hal_cycle_count_init();
    app_init();

    while (1) {
        app_run();
    }

    return 0;
}
