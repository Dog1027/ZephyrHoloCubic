#pragma once

#include <stddef.h>

/*
 * Small values kept across reboots, stored with the Zephyr settings subsystem in the
 * flash storage partition. Keys are short names like "led/base", stored under "cube/".
 */

// Read a value, returns the number of bytes read or a negative error (-ENOENT if never saved)
int prefs_load(const char *key, void *buf, size_t len);

// Write a value (a flash write, so only on user actions, never every frame)
int prefs_save(const char *key, const void *buf, size_t len);
