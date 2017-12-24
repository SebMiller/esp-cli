
#include "sdkconfig.h"

#if defined(CONFIG_CLI_USE_CMD_BLUETOOTH)

#include <string.h>
#include <stdlib.h>

#include "../cmd_create.h"
#include "../cli.h"

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_defs.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"


CLI_CMD(ble) {
    if ( CMD_HAS_ARG_AT(1, "on") ) {
        cli_printf("BLE on\n");
        return CLI_CMD_RETURN_OK;
    }

    cli_printf("Usage: %s [CMD] [OPTION]...\n", argv[0]);
    cli_printf("Control BLE.\n");
    cli_printf("More information about each command can be shown using the -h option with the command.\n");
    cli_printf("\n");
    cli_printf("CMD:\n");
    cli_printf("  on\n");

    if ( argc == 1  ||  CMD_HAS_ARG_AT(1, "-h") ) {  // print usage
        return CLI_CMD_RETURN_OK;
    }

    return CLI_CMD_RETURN_ARG_ERROR;
}

#endif
