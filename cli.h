
#ifndef CLI_H__
#define CLI_H__

#include "esp_system.h"
#include "esp_log.h"


typedef int (*flush_fc_t)(void);
int flush_default(void);

typedef struct {
    uint8_t delimiter;
    vprintf_like_t log_print_func;
    flush_fc_t log_flush_func;
    vprintf_like_t cli_print_func;
    flush_fc_t cli_flush_func;
} cli_init_t;
#ifdef CONFIG_CLI_ENABLED
#define CLI_INIT_DEFAULT() {  \
    .delimiter = '$',  \
    .log_print_func = &vprintf,  \
    .log_flush_func = &flush_default,  \
    .cli_print_func = &vprintf,  \
    .cli_flush_func = &flush_default  \
}
#else
#define CLI_INIT_DEFAULT() {0}; _Static_assert(0, "Please enable the CLI in menuconfig to use esp_cli.h")
#endif


void esp_cli_init(cli_init_t init);

int cli_printf(const char* format, ...);
int cli_vprintf(const char* format, va_list args);


#endif //CLI_H__
