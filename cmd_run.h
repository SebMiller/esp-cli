
#ifndef CMD_RUN_H__
#define CMD_RUN_H__


#include "esp_system.h"

int cli_cmd_run(bool async, char* cmd_str);

#define CLI_RUN(cmd) cli_cmd_run(false, cmd)
#define CLI_RUN_ASYNC(cmd) cli_cmd_run(true, cmd)


#endif //CMD_RUN_H__
