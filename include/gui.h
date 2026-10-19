#pragma once

// Draw the first frame and turn the display on. Call from the LVGL thread once it runs,
// never from SYS_INIT (the LVGL flush thread only starts after all SYS_INIT functions).
void gui_start(void);
