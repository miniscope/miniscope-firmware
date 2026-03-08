#include "sam.h"
#include "board.h"
#include "app.h"

int main(void)
{
    /* SystemInit() is called by the DFP startup code before main */
    board_init();
    app_init();

    while (1) {
        app_run();
    }

    return 0;
}
