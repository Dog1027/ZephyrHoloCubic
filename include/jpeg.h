#pragma once

/*
 * Decode a baseline JPEG and write it straight to the display, centered.
 * This bypasses LVGL: call jpeg_begin() once LVGL has nothing left to redraw,
 * and invalidate the screen when switching back to LVGL drawing.
 */

// Make LVGL finish drawing now, after this the screen belongs to jpeg_show()
void jpeg_begin(void);

// Returns 0 on success, -ENOENT if the file does not exist, -EIO on decode errors
int jpeg_show(const char *path);
