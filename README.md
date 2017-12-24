# Command Line Interface (CLI) for ESP-IDF

This component for ESP-IDF can be added to any project to include a Command Line Interface, with some commands already included and the possibility to write your own commands.

Currently, as well as the CLI system itself and the definitions to create and run commands, this module has a few commands already, related to the general system (the `help` command can be typed in the command line to show all available commands).

This command line interface can be an alternative to the `console` component available from ESP-IDF.


## Installation

This repository can be added as a git submodule in the `component` folder of a project. It can also be simply downloaded and moved to the `component` directory.

Configuration options can be accessed through the `make menuconfig` interface (under `Component config > CLI`).


## Configuration

#### CLI task name
The name for the CLI task.

#### CLI task stack size
The stack size of the CLI task.

#### CLI task priority
The priority of the CLI task.

#### Enable the use of ANSI escape codes
Use ANSI escape codes.
In particular, this is required for using arrows, as these are passed as ANSI escape codes.

#### Enable command history
Enable the use of the command history (Up and Down arrows).

#### Command history length
The maximum number of command that are kept in the history.

#### Command line maximum length
The maximum number of characters that can be used in one command.

#### Enable auto-completion
Enable command auto-completion using TAB.

#### Include macros for creating custom commands
Exposes macros that enable the creation of new commands.

#### Include macros for running and calling commands
Exposes macros that enable the call and run of existing commands.

#### Include CLI commands from the CLI component
Include or exclude command categories.


## Usage


### Initializing the module

Initialization is done by passing an initialization structure to the initialization function.

The initialization structure is defined as follows:
```c
typedef struct {
    uint8_t delimiter;
    vprintf_like_t log_print_func;
    flush_fc_t log_flush_func;
    vprintf_like_t cli_print_func;
    flush_fc_t cli_flush_func;
} cli_init_t;
```
With:
- `delimiter`: One ASCII character that is printed at the front of the command line.
- `log_print_func`: A `vprintf`-like function to print the ESP_LOG output.
- `log_flush_func`: An `int (void)` function that flushes the ESP_LOG output.
- `cli_print_func`: A `vprintf`-like function to print the command line and its output.
- `cli_flush_func`: An `int (void)` function that flushes the CLI output.

The default setup can be used by calling `CLI_INIT_DEFAULT()`. This will set the delimiter to `$`, both print functions to `vprintf`, and both flush functions to `flush_default` (this function simply calls `fflush(NULL)` and returns it's value).

Log output and CLI output can be different.

The log print function and log flush function can be set to NULL. This will hide all log output starting from when the CLI module is initialized.

Example:
```c
void app_main() {
    cli_init_t cli_init = CLI_INIT_DEFAULT();
    esp_cli_init(cli_init);
}
```


### Creating a command

Commands can be easily created using the CLI_CMD* set of macros:
- `CLI_CMD(command)` creates a command with a priority of 10 and a stack size of 2048 bytes.
- `CLI_CMD_STACK(command, stack)` creates a command with a priority of 10 and the given stack size.
- `CLI_CMD_PRIORITY(command, priority)` creates a command with the given priority and a stack size of 2048 bytes.
- `CLI_CMD_STACK_PRIORITY(command, stack, priority)` creates a command with the given priority and the given stack size.

Arguments:
- `command`: the name of the new command.
- `stack`: the stack size in bytes.
- `priority`: the FreeRTOS priority.

The command name needs to follow the same syntactic rules as for function and variable names in C language.

Example:
```c
CLI_CMD(hello_world) {
    cli_printf("Hello World !!\n");
    return CLI_CMD_RETURN_OK;
}
```
Output:
```
$ hello_world
Hello World !!
```


### Writing a command

When writing a command, two arguments are available: `int argc` and `char** argv`. These work exactly in the same way as the arguments passed to any `main()` function in C, with `argc` the total count of arguments, and `argv` the list of arguments as strings. The first value in `argv` is always the command name.

A command always returns an `int`. Three special return values are already defined, and can be used when writing a command. These return values are ignored when running the command from the command line.
- `CLI_CMD_RETURN_OK = 0`: There was no problem during command execution.
- `CLI_CMD_RETURN_ARG_ERROR = -1`: A required argument is missing or an argument is invalid.
- `CLI_CMD_RETURN_ERROR = -2`: Another error occurred.

Three other return values can be returned by the command run function (See "Running a command"), and should not be returned by a command:
- `-0x11`
- `-0x12`
- `-0x13`

To print any text from the command, please use the `cli_printf` function. It has the same prototype as `printf`, and uses the function provided during the module initialization under `cli_print_func`.

Examples:
```c
CLI_CMD(usage) {
    cli_printf("The command '%s' is run with the following %d arguments:\n", argv[0], argc-1);
    for (int i=1 ; i<argc ; i++) {
        cli_printf("%s\n", argv[i]);
    }
    return CLI_CMD_RETURN_OK;
}
```
Output:
```
$ usage --opt 1 -a 234
The command 'usage' is run with the following 4 arguments:
--opt
1
-a
234
```
Other examples can be found in the `commands` folder.


### Running a command

Any command can be called from the user code, or from a different command, as well as from the command line. The following macros are available for this:
- `CLI_RUN(cmd)`: Call a command synchronously. This will search for the command and run it in a separate thread and wait for the thread to finish. Returns a runtime error code, or the return value of the command.
- `CLI_RUN_ASYNC(cmd)`: Call a command asynchronously. This will search for the command and run it in a separate thread. Returns a runtime error code or `CLI_CMD_RETURN_OK`.

Runtime error codes:
- `CLI_CMD_RETURN_CMD_NOT_FOUND = -0x11`: The command name was not found.
- `CLI_CMD_RETURN_ASYNC_TIMEOUT = -0x12`: The command took too long to launch (timeout is 100ms).
- `CLI_CMD_RETURN_RUNTIME_ERROR = -0x13`: An error occurred when trying to run the command.

```c
void app_main(void) {
    esp_event_loop_init(event_handler, NULL);

    cli_init_t cli_init = CLI_INIT_DEFAULT();
    esp_cli_init(cli_init);

    int ret;
    ret = CLI_RUN("usage --run --origin main");
    ESP_LOGI("MAIN", "Run return = %d", ret);
    ret = CLI_RUN_ASYNC("usage --async --origin main");
    ESP_LOGI("MAIN", "Run return = %d", ret);
}
```
Output:
```
The command 'usage' is run with the following 3 arguments:
--run
--origin
main
I (33) MAIN: Run return = 0
The command 'usage' is run with the following 3 arguments:
 I (34) MAIN: Run return = 0
--async
--origin
main
```
Notice that running a command asynchronously will sometimes mess the output a little.


### Parsing arguments in a command

Two utility functions are available to simply parse command arguments:
- `CMD_HAS_ARG(option)`: Returns a boolean that indicates if the option (given as a string) is passed to the command.
- `CMD_HAS_ARG_AT(index, option)`: Same as above, but checks only at the given index (index 1 is the first argument passed to the command, index 2 the second, etc...).
- `CMD_ARG_VALUE(option)`: Returns a pointer to the value of the option (basically a pointer to the `argv` entry following the one of the option), or a pointer to an empty string if nothing is found.

```c
CLI_CMD(usage) {
    if ( CMD_HAS_ARG("--origin") ) {
        cli_printf("Argument '--origin' is present\n");
    }
    else {
        cli_printf("Argument '--origin' is not present\n");
    }
    cli_printf("Argument value is: %s\n", CMD_ARG_VALUE("--origin"));
    return CLI_CMD_RETURN_OK;
}
```
Output:
```
$ usage --origin main
Argument '--origin' is present
Argument value is: main
$ usage --run
Argument '--origin' is not present
Argument value is:
```
