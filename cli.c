
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "cli.h"
#include "cmd_run.h"
#include "cmd_create.h"



#define CLI_TASK_NAME CONFIG_CLI_TASK_NAME

#define CLI_TASK_STACK CONFIG_CLI_TASK_STACK

#define CLI_TASK_PRI CONFIG_CLI_TASK_PRI

#if defined(CONFIG_CLI_ANSI_ESCAPE_CODE_ENABLED)
#define CLI_ANSI_ESCAPE_CODE_ENABLED 1
#else
#define CLI_ANSI_ESCAPE_CODE_ENABLED 0
#endif

#if defined(CONFIG_CLI_HISTORY_ENABLED) && CLI_ANSI_ESCAPE_CODE_ENABLED==1
#define CLI_HISTORY_ENABLED 1
#else
#define CLI_HISTORY_ENABLED 0
#endif

#if CLI_HISTORY_ENABLED==1
#define CLI_HISTORY_LEN (CONFIG_CLI_HISTORY_LEN+1)
#else
#define CLI_HISTORY_LEN (1+1)
#endif

#define CLI_MAX_LENGTH CONFIG_CLI_MAX_LEN

#define CLI_AUTOCOMPLETE_ENABLED CONFIG_CLI_AUTOCOMPLETE_ENABLED


static char data_buff[CLI_HISTORY_LEN*CLI_MAX_LENGTH];
struct cli_status_s {
    bool inited;
    uint8_t delimiter;
    int current_length;
    int current_pos;
    int current_hist;
    bool insert;
    char* data[CLI_HISTORY_LEN];
    vprintf_like_t log_print_func;
    flush_fc_t log_flush_func;
    vprintf_like_t cli_print_func;
    flush_fc_t cli_flush_func;
    TaskHandle_t task_handle;
    bool running_sync_command;
};
static struct cli_status_s cli_status;


extern cli_funct_info_t __cli_commands_start[], __cli_commands_end[];


int log_vprintf(const char* format, va_list args);

int cli_output(const char* format, ...);

void draw_cli(void);
void clear_cli(void);
void redraw_cli(void);

void cli_task(void);


int flush_default(void) {
    return fflush(NULL);
}


void esp_cli_init(cli_init_t init) {
    if ( cli_status.inited ) {
        cli_printf("WARNING!! Detected a second attempt to init the CLI.\nThe CLI has already been inited, so this attempt will be ignored.\n");
        return;
    }
    esp_log_set_vprintf(log_vprintf);

    cli_status.delimiter = init.delimiter;
    cli_status.current_length = 0;
    cli_status.current_pos = 0;
    cli_status.current_hist = 0;
    cli_status.insert = false;
    for (int i=0 ; i<CLI_HISTORY_LEN ; i++) {
        cli_status.data[i] = data_buff+CLI_MAX_LENGTH*i;
        memset(cli_status.data[i], 0, CLI_MAX_LENGTH);
    }
    cli_status.log_print_func = init.log_print_func;
    cli_status.log_flush_func = init.log_flush_func;
    if ( init.cli_print_func != NULL ) {
        cli_status.cli_print_func = init.cli_print_func;
    }
    else {
        ESP_LOGE("CLI", "The CLI print function cannot be set to NULL.");
    }
    if ( init.cli_flush_func != NULL ) {
        cli_status.cli_flush_func = init.cli_flush_func;
    }
    else {
        ESP_LOGE("CLI", "The CLI flush function cannot be set to NULL.");
    }

    cli_status.running_sync_command = false;

    draw_cli();

    xTaskCreate((TaskFunction_t)cli_task, CLI_TASK_NAME, CLI_TASK_STACK, NULL, CLI_TASK_PRI, &(cli_status.task_handle));

    cli_status.inited = true;
}


/* Logging redirection */
int log_vprintf(const char* format, va_list args) {
    // Clear and draw CLI only if the logging is output on the same interface
    if ( cli_status.log_print_func == cli_status.cli_print_func  &&  !cli_status.running_sync_command ) {
        clear_cli();
    }
    int ret = 0;
    if ( cli_status.log_print_func != NULL ) {
        ret = cli_status.log_print_func(format, args);
        if ( cli_status.log_flush_func != NULL ) {
            cli_status.log_flush_func();
        }
    }
    if ( cli_status.log_print_func == cli_status.cli_print_func  &&  !cli_status.running_sync_command ) {
        draw_cli();
    }
    return ret;
}

/* CLI printing */
int cli_printf(const char* format, ...) {
    va_list list;
    va_start(list, format);
    int ret = cli_vprintf(format, list);
    va_end(list);
    return ret;
}
int cli_vprintf(const char* format, va_list args) {
    if ( !cli_status.running_sync_command ) {
        clear_cli();
    }
    int ret = cli_status.cli_print_func(format, args);
    if ( !cli_status.running_sync_command ) {
        draw_cli();
    }
    return ret;
}
int cli_output(const char* format, ...) {
    va_list list;
    va_start(list, format);
    int ret = cli_status.cli_print_func(format, list);
    va_end(list);
    return ret;
}
void draw_cli(void) {
    cli_output("%c %.*s", cli_status.delimiter, cli_status.current_length, cli_status.data[cli_status.current_hist]);
#if CLI_ANSI_ESCAPE_CODE_ENABLED==1
    for (int i=cli_status.current_pos ; i<cli_status.current_length ; i++) {
        cli_output("\033[1D");
    }
#endif
    cli_status.cli_flush_func();
}
void clear_cli(void) {
#if CLI_ANSI_ESCAPE_CODE_ENABLED==1
    cli_output(" ");
    int pos = cli_status.current_pos;
    while ( pos < cli_status.current_length ) {
        cli_output("\033[1C");
        pos++;
    }
    for (int i=0 ; i<cli_status.current_length+2+1 ; i++) {
        cli_output("\033[1D\033[1D");
        cli_output(" ");
    }
    cli_output("\033[1D");
#else
    cli_output("\r");
    for (int i=0 ; i<CLI_MAX_LENGTH+2 ; i++) {
        cli_output(" ");
    }
    cli_output("\r");
#endif //CLI_ANSI_ESCAPE_CODE_ENABLED==1
}
void redraw_cli(void) {
    clear_cli();
    draw_cli();
}


/* CLI manipulation */
void cli_add_char_at(int pos, uint8_t val, bool overwrite) {
    if (cli_status.current_length < CLI_MAX_LENGTH-1  &&  pos <= cli_status.current_length) {
        clear_cli();
#if CLI_HISTORY_ENABLED==1
        if (cli_status.current_hist > 0) {
            memcpy(cli_status.data[0], cli_status.data[cli_status.current_hist], CLI_MAX_LENGTH);
            cli_status.current_hist = 0;
        }
#endif //CLI_HISTORY_ENABLED==1
        if (pos == cli_status.current_length) {
            overwrite = false;
        }
        if (!overwrite) {
            for (int i=cli_status.current_length ; i>=0 && i>pos ; i--) {
                cli_status.data[cli_status.current_hist][i] = cli_status.data[cli_status.current_hist][i-1];
            }
        }
        cli_status.data[cli_status.current_hist][pos] = val;
        cli_status.current_pos++;
        if (!overwrite) {
            cli_status.current_length++;
        }
        draw_cli();
    }
}

void cli_remove_char_at(int pos, bool move_back) {
    if (cli_status.current_length > 0  &&  ((move_back && cli_status.current_pos>0) || (!move_back && cli_status.current_pos>=0))  &&  pos < cli_status.current_length) {
        clear_cli();
#if CLI_HISTORY_ENABLED==1
        if (cli_status.current_hist > 0) {
            memcpy(cli_status.data[0], cli_status.data[cli_status.current_hist], CLI_MAX_LENGTH);
            cli_status.current_hist = 0;
        }
#endif //CLI_HISTORY_ENABLED==1
        for (int i=pos ; i<cli_status.current_length-1 ; i++) {
            cli_status.data[cli_status.current_hist][i] = cli_status.data[cli_status.current_hist][i+1];
        }
        cli_status.data[cli_status.current_hist][cli_status.current_length-1] = 0;
        if (move_back) {
            cli_status.current_pos--;
        }
        cli_status.current_length--;
        draw_cli();
    }
}

#if CLI_HISTORY_ENABLED==1
void up_history() {
    if (cli_status.current_hist < CLI_HISTORY_LEN-1) {
        if (strlen((char*)cli_status.data[cli_status.current_hist+1]) > 0) {
            clear_cli();
            cli_status.current_hist++;
            cli_status.current_length = strlen((char*)cli_status.data[cli_status.current_hist]);
            cli_status.current_pos = cli_status.current_length;
            draw_cli();
        }
    }
}

void down_history() {
    if (cli_status.current_hist > 0) {
        clear_cli();
        cli_status.current_hist--;
        cli_status.current_length = strlen((char*)cli_status.data[cli_status.current_hist]);
        cli_status.current_pos = cli_status.current_length;
        draw_cli();
    }
}
#else
void up_history() {}
void down_history() {}
#endif //CLI_HISTORY_ENABLED==1

#if CLI_AUTOCOMPLETE_ENABLED==1
int autocomplete (int tab_cnt) {
    if (strlen((char*)cli_status.data[cli_status.current_hist]) == 0) {
        return tab_cnt;
    }
    else {
        for (int i=0 ; i<cli_status.current_pos ; i++) {
            if ( cli_status.data[cli_status.current_hist][i] == ' ' ) {
                return tab_cnt;
            }
        }

        cli_funct_info_t* cmd_info;
        int res_cnt = 0;
        char* complete = NULL;
        for (cmd_info=__cli_commands_start ; cmd_info<__cli_commands_end ; cmd_info++) {
            if ( strncmp(cli_status.data[cli_status.current_hist], cmd_info->name, cli_status.current_pos) == 0 ) {
                if (res_cnt == 0) {
                    if ( tab_cnt > 1 ) {
                        cli_output("\n");
                    }
                    else {
                        complete = malloc( strlen(cmd_info->name) + 1 );
                        strcpy(complete, cmd_info->name);
                    }
                }
                else if ( tab_cnt == 1 ) {
                    int len1 = strlen(complete)+1;
                    int len2 = strlen(cmd_info->name)+1;
                    for (int i=0 ; i<len1 && i<len2 ; i++) {
                        if (complete[i] != cmd_info->name[i]) {
                            complete[i] = '\0';
                            break;
                        }
                    }
                }

                if ( tab_cnt > 1 ) {
                    cli_printf("%s\n", cmd_info->name);
                }

                res_cnt++;
            }
        }

        redraw_cli();

        if ( tab_cnt == 1  &&  complete != NULL ) {
            int len = strlen(complete);
            if (cli_status.current_pos < len) {
                tab_cnt = 0;
            }
            for (int i=cli_status.current_pos ; i<len ; i++) {
                cli_add_char_at(cli_status.current_pos, complete[i], false);
            }
            if (res_cnt == 1) {
                cli_add_char_at(cli_status.current_pos, ' ', false);
            }
        }

        if (complete != NULL) {
            free(complete);
        }

        return tab_cnt;
    }
}
#else
int autocomplete (int tab_cnt) { return tab_cnt; }
#endif //CLI_AUTOCOMPLETE_ENABLED==1


/* CLI task utilities */
void parse_cmd_line() {
    if (strlen((char*)cli_status.data[cli_status.current_hist]) == 0) {
        cli_output("\n");
        redraw_cli();
    }
    else {
        cli_output("\n");
        clear_cli();
        if (cli_status.current_hist > 0) {
            memcpy(cli_status.data[0], cli_status.data[cli_status.current_hist], CLI_MAX_LENGTH);
        }
        char* tmp = cli_status.data[CLI_HISTORY_LEN-1];
        for (int i=CLI_HISTORY_LEN-1 ; i>0 ; i--) {
            cli_status.data[i] = cli_status.data[i-1];
        }
        cli_status.data[0] = tmp;
        memset(cli_status.data[0], 0, CLI_MAX_LENGTH);
        cli_status.current_hist = 0;
        cli_status.current_length = strlen((char*)cli_status.data[cli_status.current_hist]);
        cli_status.current_pos = cli_status.current_length;
        int cmd_run_len = strlen(cli_status.data[1]);
        bool async = false;
        for (int i=cmd_run_len-1 ; i>0 ; i--) {
            if ( cli_status.data[1][i] == '&' ) {
                async = true;
                break;
            }
            else if ( cli_status.data[1][i] != ' ' ) {
                break;
            }
        }
        int ret;
        if ( async ) {
            ret = CLI_RUN_ASYNC(cli_status.data[1]);
        }
        else {
            cli_status.running_sync_command = true;
            ret = CLI_RUN(cli_status.data[1]);
        }
        if ( ret == CLI_CMD_RETURN_ASYNC_TIMEOUT  ||  ret == CLI_CMD_RETURN_RUNTIME_ERROR ) {
            cli_printf("Error running the command...\n");
        }
        else if ( ret == CLI_CMD_RETURN_CMD_NOT_FOUND ) {
            cli_printf("Command not found\n");
        }
        cli_status.running_sync_command = false;
        redraw_cli();
    }
}

#if CLI_ANSI_ESCAPE_CODE_ENABLED==1
#define SPECIAL_CMD_MAX_LEN 3
int process_special_command(uint8_t val) {
    static int special_cmd = 0;
    static uint8_t special_cmd_str[SPECIAL_CMD_MAX_LEN] = {0};
    static uint8_t special_cmd_ptr = 0;

    special_cmd_str[special_cmd_ptr] = val;
    switch (special_cmd_str[special_cmd_ptr]) {
        case '[': {
            if (special_cmd_ptr == 0) {
                special_cmd = 1;
            }
            else {
                special_cmd = -1;
            }
        }
        break;
        case '2': {  // followed by '~' is insert
            if (special_cmd_ptr != 1) {
                special_cmd = -1;
            }
        }
        case '3': {  // followed by '~' is delete
            if (special_cmd_ptr != 1) {
                special_cmd = -1;
            }
        }
        break;
        case '~': {
            if (special_cmd_ptr == 2  &&  special_cmd_str[1] == '2') {  // insert
                cli_status.insert = !cli_status.insert;
                special_cmd = 2;
            }
            else if (special_cmd_ptr == 2  &&  special_cmd_str[1] == '3') {  // delete
                cli_remove_char_at(cli_status.current_pos, false);
                special_cmd = 2;
            }
            else {
                special_cmd = -1;
            }
        }
        break;
        case 'A': {  // up arrow
            up_history();
            special_cmd = 2;
        }
        break;
        case 'B': {  // down arrow
            down_history();
            special_cmd = 2;
        }
        break;
        case 'C': {  // right arrow
            if (cli_status.current_pos < cli_status.current_length) {
                cli_status.current_pos++;
            }
            special_cmd = 2;
        }
        break;
        case 'D': {  // left arrow
            if (cli_status.current_pos > 0) {
                cli_status.current_pos--;
            }
            special_cmd = 2;
        }
        break;
        case 0xff: {
            special_cmd_ptr--;  // ignore this character
        }
        break;
        default: {
            special_cmd = -1;
        }
        break;
    }

    special_cmd_ptr++;

    if (special_cmd == 2) {  // special command was executed, forget characters
        special_cmd_ptr = SPECIAL_CMD_MAX_LEN;
    }
    else if (special_cmd == -1) {  // the string was not a special command
        for (int i=0 ; i<special_cmd_ptr ; i++) {
            cli_add_char_at(cli_status.current_pos, special_cmd_str[i], cli_status.insert);
        }
        special_cmd_ptr = SPECIAL_CMD_MAX_LEN;
    }

    if (special_cmd_ptr == SPECIAL_CMD_MAX_LEN) {  // too many characters for a special command or command done
        for (int i=0 ; i<SPECIAL_CMD_MAX_LEN ; i++) {
            special_cmd_str[i] = 0;
        }
        special_cmd_ptr = 0;
        special_cmd = 0;
        redraw_cli();
    }

    return special_cmd;
}
#else
int process_special_command(uint8_t val) { return 0; }
#endif //CLI_ANSI_ESCAPE_CODE_ENABLED==1

void process_char(uint8_t val) {
    static int special_cmd = 0;
    static int tab_count = 0;

    if (val == 0x08) {  // backspace
        cli_remove_char_at(cli_status.current_pos-1, true);
    }

    if (val == 0x09) {  // tab
        tab_count++;
        tab_count = autocomplete(tab_count);
    }
    else if (0x01 <= val  &&  val <= 0xfe) {
        tab_count = 0;
    }

    if (val == 0x0A) {  // new line
        parse_cmd_line();
    }

    if (val == 0x0D) {  // carriage return
        cli_printf("Carriage return\n");
    }

#if CLI_ANSI_ESCAPE_CODE_ENABLED==1
    if (val == '[') {
        special_cmd = 1;
    }
    else
#endif //CLI_ANSI_ESCAPE_CODE_ENABLED==1
    if (0x20 <= val  &&  val <= 0x7f) {
        if (!special_cmd) {
            cli_add_char_at(cli_status.current_pos, val, cli_status.insert);
        }
    }

#if CLI_ANSI_ESCAPE_CODE_ENABLED==1
    if (special_cmd) {
        special_cmd = process_special_command(val);
    }
#endif //CLI_ANSI_ESCAPE_CODE_ENABLED==1
}

/* CLI task */
void cli_task() {
    while (1) {
        uint8_t in = getchar();
        process_char(in);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
