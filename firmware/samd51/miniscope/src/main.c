#include "board_common.h"
#include "app.h"

int main(void)
{
    board_init();
    app_init();

    while (1) {
        app_run();
    }

    return 0;
}
