#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <ff.h>
#include "file.h"

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

static int lsdir(const char* path)
{
    int res;
    struct fs_dir_t dirp;
    static struct fs_dirent entry;
    int count = 0;

    fs_dir_t_init(&dirp);

    /* Verify fs_opendir() */
    res = fs_opendir(&dirp, path);
    if (res) {
        printk("Error opening dir %s [%d]\n", path, res);
        return res;
    }

    printk("\nListing dir %s ...\n", path);
    for (;;) {
        /* Verify fs_readdir() */
        res = fs_readdir(&dirp, &entry);

        /* entry.name[0] == 0 means end-of-dir */
        if (res || entry.name[0] == 0) {
            break;
        }

        if (entry.type == FS_DIR_ENTRY_DIR) {
            printk("[DIR ] %s\n", entry.name);
        }
        else {
            printk("[FILE] %s (size = %zu)\n", entry.name, entry.size);
        }
        count++;
    }

    /* Verify fs_closedir() */
    fs_closedir(&dirp);
    if (res == 0) {
        res = count;
    }

    return res;
}

static int file_init(void)
{
    int rc;

    rc = fs_mount(&sd_mnt);
    if (rc < 0) {
        printk("Failed to mount SD card (%d)\n", rc);
        return rc;
    }

    printk("SD card mounted at %s\n", sd_mnt.mnt_point);

    rc = lsdir("/0:");
    if (rc < 0) {
        printk("Failed to list directory (%d)\n", rc);
        return rc;
    }

    return 0;
}

SYS_INIT(file_init, APPLICATION, CONFIG_FILE_INIT_PRIORITY);
