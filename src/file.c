#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
#include <ff.h>
#include "file.h"

LOG_MODULE_REGISTER(file, CONFIG_GUI_LOG_LEVEL);

#define MKFS_FS_TYPE FS_FATFS

static FATFS sd_fatfs;
static struct fs_mount_t sd_mnt = {
    .type = FS_FATFS,
    .mnt_point = "/0:",
    .fs_data = &sd_fatfs,
    .storage_dev = (void *)"SD",
};

bool file_exists(const char *path)
{
    // fs_dirent 帶著 256 bytes 的檔名，放 static 省 stack；只有 app thread 會呼叫
    static struct fs_dirent entry;
    return fs_stat(path, &entry) == 0;
}

static int file_init(void)
{
    int rc;

    rc = fs_mount(&sd_mnt);
    if (rc < 0) {
        LOG_ERR("Failed to mount SD card (%d)", rc);
        return rc;
    }

    LOG_INF("SD card mounted at %s", sd_mnt.mnt_point);
    return 0;
}

SYS_INIT(file_init, APPLICATION, CONFIG_FILE_INIT_PRIORITY);
