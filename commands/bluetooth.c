
#include "sdkconfig.h"

#if defined(CONFIG_CLI_USE_CMD_BLUETOOTH)

#include <string.h>
#include <stdlib.h>

#include "../cmd_create.h"
#include "../cli.h"

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_defs.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"


static bool bluetooth_cmd_ble_is_inited = false;
static bool bluetooth_cmd_bt_classic_is_released = false;
static bool bluetooth_cmd_controller_is_inited = false;
static bool bluetooth_cmd_controller_is_enabled = false;
static bool bluetooth_cmd_bluedroid_is_inited = false;
static bool bluetooth_cmd_bluedroid_is_enabled = false;
static bool bluetooth_cmd_gap_evt_handler_registered = false;


static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);


static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x0020,
    .adv_int_max        = 0x0800,
    .adv_type           = ADV_TYPE_SCAN_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY
};


CLI_CMD(ble) {
    if ( CMD_HAS_ARG_AT(1, "on") ) {
        esp_err_t ret;

        if ( CMD_HAS_ARG("-h") ) {
            cli_printf("Usage: %s %s [OPTION]...\n", argv[0], argv[1]);
            cli_printf("Initialize and start the bluetooth controller and stack.\n");
            cli_printf("\n");
            cli_printf("OPTION:\n");
            cli_printf("  -h: show this help.\n");
            cli_printf("  -n NAME: the device name (will default to ESP_CLI_BLE if not specified).\n");
            return CLI_CMD_RETURN_OK;
        }

        if (bluetooth_cmd_ble_is_inited) {
            cli_printf("BLE has already been turned on\n");
            return CLI_CMD_RETURN_ERROR;
        }

        if ( !bluetooth_cmd_bt_classic_is_released ) {
            ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
            if (ret) {
                cli_printf("BT classic mem release failed\n");
                return CLI_CMD_RETURN_ERROR;
            }
            bluetooth_cmd_bt_classic_is_released = true;
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
        if ( !bluetooth_cmd_gap_evt_handler_registered ) {
            ret = esp_ble_gap_register_callback(gap_event_handler);
            if (ret) {
                cli_printf("GAP event handler register failed\n");
                return CLI_CMD_RETURN_ERROR;
            }
            bluetooth_cmd_gap_evt_handler_registered = true;
        }

        if ( CMD_HAS_ARG("-n") ) {
            ret = esp_ble_gap_set_device_name( CMD_ARG_VALUE("-n") );
        }
        else {
            ret = esp_ble_gap_set_device_name("ESP_CLI_BLE");
        }
        if (ret) {
            cli_printf("GAP set device name failed\n");
            return CLI_CMD_RETURN_ERROR;
        }

        bluetooth_cmd_ble_is_inited = true;

        cli_printf("BLE is now on\n");

        return CLI_CMD_RETURN_OK;
    }
    else if ( CMD_HAS_ARG_AT(1, "adv") ) {
        esp_err_t ret;

        if ( CMD_HAS_ARG("-h") ) {
            cli_printf("Usage: %s %s [OPTION]...\n", argv[0], argv[1]);
            cli_printf("Start BLE advertising.\n");
            cli_printf("\n");
            cli_printf("OPTION:\n");
            cli_printf("  -h: show this help.\n");
            return CLI_CMD_RETURN_OK;
        }

        ret = esp_ble_gap_start_advertising(&adv_params);
        if (ret) {
            cli_printf("GAP start advertising failed\n");
            return CLI_CMD_RETURN_ERROR;
        }

        return CLI_CMD_RETURN_OK;
    }

    cli_printf("Usage: %s [CMD] [OPTION]...\n", argv[0]);
    cli_printf("Control BLE.\n");
    cli_printf("More information about each command can be shown using the -h option with the command.\n");
    cli_printf("\n");
    cli_printf("CMD:\n");
    cli_printf("  on\n");
    cli_printf("  adv\n");

    if ( argc == 1  ||  CMD_HAS_ARG_AT(1, "-h") ) {  // print usage
        return CLI_CMD_RETURN_OK;
    }

    return CLI_CMD_RETURN_ARG_ERROR;
}


static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT: {
            cli_printf("ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT\n");
        }
        break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT: {
            cli_printf("ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT\n");
        }
        break;
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
            cli_printf("ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT\n");
        }
        break;
        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            cli_printf("ESP_GAP_BLE_SCAN_RESULT_EVT\n");
        }
        break;
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT: {
            cli_printf("ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT\n");
        }
        break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT: {
            cli_printf("ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT\n");
        }
        break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT: {
            cli_printf("ESP_GAP_BLE_ADV_START_COMPLETE_EVT\n");
        }
        break;
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: {
            cli_printf("ESP_GAP_BLE_SCAN_START_COMPLETE_EVT\n");
        }
        break;
        case ESP_GAP_BLE_AUTH_CMPL_EVT: {
            cli_printf("ESP_GAP_BLE_AUTH_CMPL_EVT\n");
        }
        break;
        case ESP_GAP_BLE_KEY_EVT: {
            cli_printf("ESP_GAP_BLE_KEY_EVT\n");
        }
        break;
        case ESP_GAP_BLE_SEC_REQ_EVT: {
            cli_printf("ESP_GAP_BLE_SEC_REQ_EVT\n");
        }
        break;
        case ESP_GAP_BLE_PASSKEY_NOTIF_EVT: {
            cli_printf("ESP_GAP_BLE_PASSKEY_NOTIF_EVT\n");
        }
        break;
        case ESP_GAP_BLE_PASSKEY_REQ_EVT: {
            cli_printf("ESP_GAP_BLE_PASSKEY_REQ_EVT\n");
        }
        break;
        case ESP_GAP_BLE_OOB_REQ_EVT: {
            cli_printf("ESP_GAP_BLE_OOB_REQ_EVT\n");
        }
        break;
        case ESP_GAP_BLE_LOCAL_IR_EVT: {
            cli_printf("ESP_GAP_BLE_LOCAL_IR_EVT\n");
        }
        break;
        case ESP_GAP_BLE_LOCAL_ER_EVT: {
            cli_printf("ESP_GAP_BLE_LOCAL_ER_EVT\n");
        }
        break;
        case ESP_GAP_BLE_NC_REQ_EVT: {
            cli_printf("ESP_GAP_BLE_NC_REQ_EVT\n");
        }
        break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT: {
            cli_printf("ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT\n");
        }
        break;
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT: {
            cli_printf("ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT\n");
        }
        break;
        case ESP_GAP_BLE_SET_STATIC_RAND_ADDR_EVT: {
            cli_printf("ESP_GAP_BLE_SET_STATIC_RAND_ADDR_EVT\n");
        }
        break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT: {
            cli_printf("ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT\n");
        }
        break;
        case ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT: {
            cli_printf("ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT\n");
        }
        break;
        case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT: {
            cli_printf("ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT\n");
        }
        break;
        case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT: {
            cli_printf("ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT\n");
        }
        break;
        case ESP_GAP_BLE_CLEAR_BOND_DEV_COMPLETE_EVT: {
            cli_printf("ESP_GAP_BLE_CLEAR_BOND_DEV_COMPLETE_EVT\n");
        }
        break;
        case ESP_GAP_BLE_GET_BOND_DEV_COMPLETE_EVT: {
            cli_printf("ESP_GAP_BLE_GET_BOND_DEV_COMPLETE_EVT\n");
        }
        break;
        case ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT: {
            cli_printf("ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT\n");
        }
        break;
        case ESP_GAP_BLE_ADD_WHITELIST_COMPLETE_EVT: {
            cli_printf("ESP_GAP_BLE_ADD_WHITELIST_COMPLETE_EVT\n");
        }
        break;
        case ESP_GAP_BLE_EVT_MAX: {
            cli_printf("ESP_GAP_BLE_EVT_MAX\n");
        }
        break;
        default:
        break;
    }
}


#endif
