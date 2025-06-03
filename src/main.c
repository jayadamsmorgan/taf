#include "cmd_parser.h"

#include "taf_init.h"
#include "taf_logs.h"
#include "taf_target.h"
#include "taf_test.h"

#include <stdlib.h>

int main(int argc, char **argv) {

    // Parse cli options
    cmd_category cmd = cmd_parser_parse(argc, argv);

    switch (cmd) {
    case CMD_INIT:
        return taf_init();
    case CMD_TEST:
        return taf_test();
    case CMD_LOGS_INFO:
        return taf_logs_info();
    case CMD_TARGET_ADD:
        return taf_target_add();
    case CMD_TARGET_REMOVE:
        return taf_target_remove();
    case CMD_HELP:
        // Should be already handled in cmd_parser
        return EXIT_SUCCESS;
    case CMD_UNKNOWN:
        // Should be already handled in cmd_parser
        return EXIT_FAILURE;
    }
}
