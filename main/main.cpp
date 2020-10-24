
#define BTSTACK_FILE__ "hid_host_demo.c"

#include "config.h"
#include "bt_manager.h"

/*************************************************************************************************/

extern "C" { int app_main(void); }
int app_main(void)
{
    // Setup
    btstack_main(0, 0);

    // Enter run loop (forever)
    btstack_run();

    return 0;
}
