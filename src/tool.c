#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include "app_mgr.h"

static int _fg(const struct shell *shell, size_t argc, char **argv)
{
    if (argc != 2) {
        shell_print(shell, "Usage: tool fg <index>");
        return -EINVAL;
    }

    int index = atoi(argv[1]);
    if (index < 1) {
        shell_print(shell, "Index must larger than 0");
        return -EINVAL;
    }

    int ret = app_mgr_load_app(index);
    if (ret != 0) {
        shell_print(shell, "Failed to load foreground app with index %d, error: %d", index, ret);
        return ret;
    }

    shell_print(shell, "Foreground app with index %d loaded", index);
    return 0;
}

static int _end_fg(const struct shell *shell, size_t argc, char **argv)
{
    int ret = app_mgr_end_app();
    if (ret != 0) {
        shell_print(shell, "Failed to end foreground app, error: %d", ret);
        return ret;
    }

    shell_print(shell, "Foreground app ended");
    return 0;
}

static int _list(const struct shell *shell, size_t argc, char **argv)
{
    shell_print(shell, "Not implemented yet");
    return 0;
}

/* Define shell subcommands */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_tool,
    SHELL_CMD_ARG(fg, NULL, "fg", _fg, 2, 0),
    SHELL_CMD_ARG(end_fg, NULL, "end_fg", _end_fg, 1, 0),
    SHELL_CMD_ARG(list, NULL, "list", _list, 1, 0),
    SHELL_SUBCMD_SET_END);

/* Register main shell command */
SHELL_CMD_ARG_REGISTER(tool, &sub_tool, "Tool commands", NULL, 1, 0);
