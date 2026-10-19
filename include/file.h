#pragma once

#include <stdbool.h>

// True if the file or folder exists. Use it before opening optional files:
// fs_open / fs_opendir log an error for missing paths, fs_stat does not.
bool file_exists(const char *path);
