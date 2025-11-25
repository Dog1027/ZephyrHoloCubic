
#include <zephyr/shell/shell.h>

#include <zephyr/fs/fs.h>
#include <ff.h>

/*
 *  Note the fatfs library is able to mount only strings inside _VOLUME_STRS
 *  in ffconf.h
 */
#define DISK_DRIVE_NAME "SD"
#define DISK_MOUNT_PT "/"DISK_DRIVE_NAME":"
#define FS_RET_OK FR_OK

static FATFS fat_fs;
/* mounting info */
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};

int _mkfs(const struct shell *shell, size_t argc, char **argv)
{
    const char *mount_point = DISK_MOUNT_PT;
    int ret;

    ret = fs_mkfs(FS_FATFS, (uintptr_t)mount_point, NULL, 0);
    if (ret != 0) {
        shell_print(shell, "Failed to format the filesystem at %s (err %d)", mount_point, ret);
        return ret;
    }

    shell_print(shell, "Filesystem at %s formatted successfully.", mount_point);
    return 0;
}

int _mount(const struct shell *shell, size_t argc, char **argv)
{
    const char *mount_point = DISK_MOUNT_PT;
    int ret;

    mp.mnt_point = mount_point;

    ret = fs_mount(&mp);
    if (ret != FS_RET_OK) {
        shell_print(shell, "Failed to mount filesystem at %s (err %d)", mount_point, ret);
        return ret;
    }

    shell_print(shell, "Filesystem mounted at %s.", mount_point);
    return 0;
}

int _unmount(const struct shell *shell, size_t argc, char **argv)
{
    const char *mount_point = DISK_MOUNT_PT;
    int ret;

    mp.mnt_point = mount_point;

    ret = fs_unmount(&mp);
    if (ret != FS_RET_OK) {
        shell_print(shell, "Failed to unmount filesystem at %s (err %d)", mount_point, ret);
        return ret;
    }

    shell_print(shell, "Filesystem unmounted at %s.", mount_point);
    return 0;
}

/* Define shell subcommands */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_tool,
    SHELL_CMD_ARG(mkfs, NULL, "mkfs", _mkfs, 1, 0),
    SHELL_CMD_ARG(mount, NULL, "mount", _mount, 1, 0),
    SHELL_CMD_ARG(unmount, NULL, "unmount", _unmount, 1, 0),
    SHELL_SUBCMD_SET_END);

/* Register main shell command */
SHELL_CMD_ARG_REGISTER(tool, &sub_tool, "Tool commands", NULL, 1, 0);
