

#ifndef ESP_CLI_H__
#define ESP_CLI_H__

#include "../cli.h"

#ifdef CONFIG_CLI_ALLOW_COMMAND_ADDITION
#include "../cmd_create.h"
#endif

#if CONFIG_CLI_ALLOW_COMMAND_RUN
#include "../cmd_run.h"
#endif

#endif //ESP_CLI_H__
