

#include "cmd_create.h"


char* get_argv_option_value(const char* option, int argc, char* argv[]) {
    for (int i=1 ; i<argc-1 ; i++) {
        if ( strcmp(option, argv[i]) == 0 ) {
            return argv[i+1];
        }
    }
    return "";
}

bool get_argv_has_option(const char* option, int argc, char* argv[]) {
    for (int i=1 ; i<argc ; i++) {
        if ( strcmp(option, argv[i]) == 0 ) {
            return true;
        }
    }
    return false;
}
