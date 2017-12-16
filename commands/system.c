
#include "sdkconfig.h"

#if defined(CONFIG_CLI_USE_CMD_SYSTEM)

#include <string.h>
#include <stdlib.h>

#include "../cmd_create.h"
#include "../cli.h"


CLI_CMD(sizeof) {
    if ( argc < 2 ) {
        cli_printf("  Usage:  sizeof <type> ...\n");
        return CLI_CMD_RETURN_ARG_ERROR;
    }

    for (int i=1 ; i<argc ; i++) {
        if ( strcmp(argv[i], "char") == 0 ) {
            cli_printf("sizeof(char) = %d\n", sizeof(char));
        }
        else if ( strcmp(argv[i], "int") == 0 ) {
            cli_printf("sizeof(int) = %d\n", sizeof(int));
        }
        else if ( strcmp(argv[i], "short") == 0 ) {
            cli_printf("sizeof(short) = %d\n", sizeof(short));
        }
        else if ( strcmp(argv[i], "long") == 0 ) {
            cli_printf("sizeof(long) = %d\n", sizeof(long));
        }
        else if ( strcmp(argv[i], "long long") == 0 ) {
            cli_printf("sizeof(long long) = %d\n", sizeof(long long));
        }
        else if ( strcmp(argv[i], "float") == 0 ) {
            cli_printf("sizeof(float) = %d\n", sizeof(float));
        }
    }

    return CLI_CMD_RETURN_OK;
}

CLI_CMD(restart) {
    cli_printf("The system will restart in a couple of seconds\n");
    CLI_CMD_SLEEP(2);
    esp_restart();
    return CLI_CMD_RETURN_OK;
}

CLI_CMD(version) {
    cli_printf("ESP-IDF %s\n", esp_get_idf_version());

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    const char* model_str = chip_info.model==CHIP_ESP32 ? "ESP32" : "";
    const char* ft_emb_flash = (chip_info.features|CHIP_FEATURE_EMB_FLASH)>0 ? " EMB_FLASH" : "";
    const char* ft_wifi_bgn = (chip_info.features|CHIP_FEATURE_WIFI_BGN)>0 ? " WIFI_BGN" : "";
    const char* ft_ble = (chip_info.features|CHIP_FEATURE_BLE)>0 ? " BLE" : "";
    const char* ft_bt = (chip_info.features|CHIP_FEATURE_BT)>0 ? " BT" : "";
    cli_printf( "Chip:\n"
                "  Model: %s\n"
                "  Features:%s%s%s%s\n"
                "  Cores: %d\n"
                "  Revision: %d\n",
        model_str, ft_emb_flash, ft_wifi_bgn, ft_ble, ft_bt, chip_info.cores, chip_info.revision);

    return CLI_CMD_RETURN_OK;
}

CLI_CMD(sleep) {
    if ( argc != 2 ) {
        cli_printf("  Usage:  sleep sec\n");
        return CLI_CMD_RETURN_ARG_ERROR;
    }

    int duration = atoi(argv[1]);
    CLI_CMD_SLEEP(duration);

    return CLI_CMD_RETURN_OK;
}

CLI_CMD(heap) {
    cli_printf("Free heap size: %d bytes\n", esp_get_free_heap_size());

    return CLI_CMD_RETURN_OK;
}

CLI_CMD(heap_min) {
    cli_printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

    return CLI_CMD_RETURN_OK;
}

extern cli_funct_info_t __cli_commands_start[], __cli_commands_end[];
CLI_CMD(help) {
    cli_funct_info_t* cmd_info;
    cli_printf("******** All available commands ********\n");
    for (cmd_info=__cli_commands_start ; cmd_info<__cli_commands_end ; cmd_info++) {
        cli_printf("%s\n", cmd_info->name);
    }
    cli_printf("********          END           ********\n");

    return CLI_CMD_RETURN_OK;
}

#endif
