
#ifndef CMD_CREATE_H__
#define CMD_CREATE_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>


typedef struct cli_funct_info_s {
    const char *name;
    int stack_size;
    int priority;
    int (*funct)(int, char**);
} cli_funct_info_t;

#define CLI_CMD_DECLARE(command, stack, pri)  \
            static const char __cli_cmd__name__##command[] __attribute__((__section__(".rodata"))) = #command;  \
            static int __attribute__((__used__)) __cli_cmd__funct__##command(int, char**);  \
            static cli_funct_info_t __cli_cmd__info__##command __attribute__((__used__)) __attribute__((__section__(".cli.commands")))  \
                = {__cli_cmd__name__##command, stack, pri, __cli_cmd__funct__##command};  \
            static int __attribute__((__used__)) __cli_cmd__funct__##command(int argc, char** argv)

#define CLI_CMD(command) CLI_CMD_DECLARE(command, 2048, 10)
#define CLI_CMD_STACK(command, stack) CLI_CMD_DECLARE(command, stack, 10)
#define CLI_CMD_PRIORITY(command, priority) CLI_CMD_DECLARE(command, 2048, priority)
#define CLI_CMD_STACK_PRIORITY(command, stack, priority) CLI_CMD_DECLARE(command, stack, priority)

#define CLI_CMD_MSSLEEP(ms) vTaskDelay( pdMS_TO_TICKS(ms) )
#define CLI_CMD_SLEEP(s) vTaskDelay( pdMS_TO_TICKS(1000*s) )


#define CLI_CMD_RETURN_RUNTIME_ERROR              -0x13
#define CLI_CMD_RETURN_ASYNC_TIMEOUT              -0x12
#define CLI_CMD_RETURN_CMD_NOT_FOUND              -0x11

#define CLI_CMD_RETURN_ERROR                      -0x02
#define CLI_CMD_RETURN_ARG_ERROR                  -0x01

#define CLI_CMD_RETURN_OK                          0



bool get_argv_has_option(const char* option, int argc, char* argv[]);
bool get_argv_has_option_at(int index, const char* option, int argc, char* argv[]);
char* get_argv_option_value(const char* option, int argc, char* argv[]);

#define CMD_HAS_ARG(option) get_argv_has_option(option, argc, argv)
#define CMD_HAS_ARG_AT(index, option) get_argv_has_option_at(index, option, argc, argv)
#define CMD_ARG_VALUE(option) get_argv_option_value(option, argc, argv)


#endif //CMD_CREATE_H__
