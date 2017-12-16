
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include <string.h>

#include "cmd_run.h"
#include "cmd_create.h"

extern cli_funct_info_t __cli_commands_start[], __cli_commands_end[];

struct async_params {
    SemaphoreHandle_t sync;
    int (*funct)(int, char**);
    bool async;
    char* cmd_str;
    int return_val;
};
void cli_cmd_task(void* vparams);

int cli_cmd_run(bool async, char* cmd_str) {
    cli_funct_info_t* cmd_info;
    int cmd_len=0;

    while ( cmd_str[cmd_len] != ' '  &&  cmd_str[cmd_len] != '\0' ) {
        cmd_len++;
    }

    if ( cmd_len == 0 ) {
        return CLI_CMD_RETURN_OK;
    }

    for (cmd_info=__cli_commands_start ; cmd_info<__cli_commands_end ; cmd_info++) {
        if ( strlen(cmd_info->name) != cmd_len ) {
            continue;
        }
        if ( strncmp(cmd_str, cmd_info->name, cmd_len) == 0 ) {
            struct async_params params;
            params.cmd_str = cmd_str;
            params.funct = cmd_info->funct;
            params.async = async;
            params.sync = xSemaphoreCreateBinary();
            xTaskCreate( cli_cmd_task, cmd_str, cmd_info->stack_size, (void*)&params, cmd_info->priority, NULL );
            if ( async ) {
                int ret = xSemaphoreTake( params.sync, pdMS_TO_TICKS(100) );
                if ( ret == pdFAIL ) {
                    return CLI_CMD_RETURN_ASYNC_TIMEOUT;
                }
                else {
                    return params.return_val;
                }
            }
            else {
                xSemaphoreTake( params.sync, portMAX_DELAY );
                return params.return_val;
            }
        }
    }

    return CLI_CMD_RETURN_CMD_NOT_FOUND;
}

void cli_cmd_task(void* vparams) {
    struct async_params* params = (struct async_params*)vparams;
    int cmd_len = strlen(params->cmd_str);

    char* command_name = NULL;
    int argc = 0;
    char** argv = NULL;

    char* command = malloc(cmd_len+1);
    if (command == NULL) {
        params->return_val = CLI_CMD_RETURN_RUNTIME_ERROR;
        xSemaphoreGive( params->sync );
        goto exit;
    }
    strcpy(command, params->cmd_str);
    command[cmd_len] = '\0';

    if ( params->async ) {
        for (int i=cmd_len-1 ; i>0 ; i--) {
            if ( command[i] == '&' ) {
                command[i] = '\0';
                break;
            }
            else if ( command[i] != ' ' ) {
                break;
            }
        }
    }

    // count non-successive unquoted spaces
    bool ignore_space = false;
    for (int i=0 ; i<cmd_len ; i++) {
        if ( i > 0 ) {
            if ( command[i] == '"'  &&  command[i-1] != '\\' ) {
                ignore_space = !ignore_space;
                command[i] = '\0';
            }
        }
        if ( !ignore_space ) {
            if ( command[i] == ' ' ) {
                if ( command[i+1] != ' '  &&  command[i+1] != '\0' ) {
                    argc++;
                }
                command[i] = '\0';
            }
        }
    }
    argc++;
    argv = malloc(argc*sizeof(char*));
    if (argv == NULL) {
        params->return_val = CLI_CMD_RETURN_RUNTIME_ERROR;
        xSemaphoreGive( params->sync );
        goto exit;
    }

    int cpy_offset = 0;
    int elem_cnt = 0;
    int arg_cnt = 0;
    for (int i=0 ; i<cmd_len+1 ; i++) {
        if ( command[i] == '\0' ) {
            if ( command_name == NULL ) {
                command_name = command+i-elem_cnt;
                argv[arg_cnt++] = command_name;
            }
            else if ( elem_cnt > 0  &&  argc > arg_cnt ) {
                argv[arg_cnt++] = command+i-elem_cnt;
            }
            elem_cnt = 0;
            cpy_offset = 0;
        }
        else {
            elem_cnt++;
            if ( command[i+cpy_offset] == '\\'  &&  command[i+cpy_offset+1] == '"' ) {
                cpy_offset++;
            }
            command[i] = command[i+cpy_offset];
        }
    }

    if ( command_name == NULL ) {
        params->return_val = CLI_CMD_RETURN_RUNTIME_ERROR;
        xSemaphoreGive( params->sync );
        goto exit;
    }

    if ( params->async ) {
        params->return_val = CLI_CMD_RETURN_OK;
        xSemaphoreGive( params->sync );
    }

    int ret = params->funct(argc, argv);

    if ( !params->async ) {
        params->return_val = ret;
        xSemaphoreGive( params->sync );
    }

exit:
    if (command != NULL) {
        free(command);
    }
    if (argv != NULL) {
        free(argv);
    }

    vTaskDelete(NULL);
    while (1) {
        vTaskDelay(1000);
    }
}
