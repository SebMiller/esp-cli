
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


static bool bluetooth_cmd_ble_is_inited = false;
static bool bluetooth_cmd_controller_is_inited = false;
static bool bluetooth_cmd_controller_is_enabled = false;
static bool bluetooth_cmd_bluedroid_is_inited = false;
static bool bluetooth_cmd_bluedroid_is_enabled = false;

CLI_CMD(ble) {
    if ( CMD_HAS_ARG_AT(1, "on") ) {
        esp_err_t ret;

        if ( CMD_HAS_ARG("-h") ) {
            cli_printf("Usage: %s on [OPTION]...\n", argv[0]);
            cli_printf("initialize and start the bluetooth controller and stack.\n");
            cli_printf("\n");
            cli_printf("OPTION:\n");
            cli_printf("  -h: show this help.\n");
            return CLI_CMD_RETURN_OK;
        }

        if (bluetooth_cmd_ble_is_inited) {
            cli_printf("BLE has already been turned on\n");
            return CLI_CMD_RETURN_ERROR;
        }

        if ( !bluetooth_cmd_controller_is_inited ) {
            esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
            ret = esp_bt_controller_init(&bt_cfg);
            if (ret) {
                cli_printf("BT controller initialize failed\n");
                return CLI_CMD_RETURN_ERROR;
            }
            bluetooth_cmd_controller_is_inited = true;
        }
        if ( !bluetooth_cmd_controller_is_enabled ) {
            ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
            if (ret) {
                cli_printf("BT controller enable failed\n");
                return CLI_CMD_RETURN_ERROR;
            }
            bluetooth_cmd_controller_is_enabled = true;
        }
        if ( !bluetooth_cmd_bluedroid_is_inited ) {
            ret = esp_bluedroid_init();
            if (ret) {
                cli_printf("Bluedroid initialize failed\n");
                return CLI_CMD_RETURN_ERROR;
            }
            bluetooth_cmd_bluedroid_is_inited = true;
        }
        if ( !bluetooth_cmd_bluedroid_is_enabled ) {
            ret = esp_bluedroid_enable();
            if (ret) {
                cli_printf("Bluedroid enable failed\n");
                return CLI_CMD_RETURN_ERROR;
            }
            bluetooth_cmd_bluedroid_is_enabled = true;
        }

        bluetooth_cmd_ble_is_inited = true;

        cli_printf("BLE is now on\n");

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
