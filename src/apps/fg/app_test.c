#include "fg_app.h"
#include "input.h"
#include <zephyr/fs/fs.h>
#include <math.h>

APP_FG_REGISTER(Test, app_test_init, app_test_run, CONFIG_APP_FG_PRIORITY_DEFAULT);

// lv_obj_t *label;
// lv_obj_t *label_ax;
// lv_obj_t *label_ay;
// lv_obj_t *label_az;
static lv_obj_t *img1;
// char buf[16];


// static void key_event_handler(lv_event_t * e)
// {
//     lv_indev_t * indev = lv_indev_active();
//     char key = lv_indev_get_key(indev);
//     if (key == '\0') {
//         return;
//     }
//     buf[0] = key;
//     lv_label_set_text(label, buf);
// }

#define IMG_RESOLUTION 168
static uint8_t img_data_buffer[IMG_RESOLUTION * IMG_RESOLUTION * 2]; // 168x168 像素，RGB565 每像素 2 字节
static lv_image_dsc_t ram_img_dsc;

void load_img_to_ram(lv_obj_t *img) {
    struct fs_file_t file;
    fs_file_t_init(&file);
    
    // 從 SD 卡讀取原始數據 (跳過你檔案中那個奇怪的 Header，從像素數據開始讀)
    if (fs_open(&file, "/0:/image/img1.bin", FS_O_READ) == 0) {
        fs_read(&file, img_data_buffer, sizeof(img_data_buffer));
        fs_close(&file);
        
        // 2. 手動構造一個 LVGL 認得的 RAM 描述符
        ram_img_dsc.header.w = IMG_RESOLUTION;
        ram_img_dsc.header.h = IMG_RESOLUTION;
        ram_img_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
        ram_img_dsc.header.stride = IMG_RESOLUTION * 2;
        ram_img_dsc.data = img_data_buffer;
        ram_img_dsc.data_size = sizeof(img_data_buffer);
        
        // 3. 將圖片來源指向這個 RAM 結構體
        lv_image_set_src(img, &ram_img_dsc);
        printk("圖片已載入 RAM，現在旋轉應該會非常絲滑！\n");
    }
}

static void app_test_init(void *scr)
{
    // Initialization code for foreground test app
    // label = lv_label_create(scr);
    // lv_label_set_text(label, "Press keys...");
    // lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);
    // lv_obj_add_event_cb(label, key_event_handler, LV_EVENT_KEY, NULL);

    // lv_group_t *g = lv_group_get_default();
    // lv_obj_t *ta;
    // ta = lv_textarea_create(scr);
    // lv_textarea_set_one_line(ta, true);
    // lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 10);
    // lv_group_add_obj(g, ta);

    // ta = lv_textarea_create(scr);
    // lv_textarea_set_one_line(ta, true);
    // lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 60);
    // lv_group_add_obj(g, ta);

    // ta = lv_textarea_create(scr);
    // lv_textarea_set_one_line(ta, true);
    // lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 110);
    // lv_group_add_obj(g, ta);

    // ta = lv_textarea_create(scr);
    // lv_textarea_set_one_line(ta, true);
    // lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 160);
    // lv_group_add_obj(g, ta);

    // label_ax = lv_label_create(scr);
    // lv_obj_align(label_ax, LV_ALIGN_TOP_LEFT, 30, 80);
    // label_ay = lv_label_create(scr);
    // lv_obj_align(label_ay, LV_ALIGN_TOP_LEFT, 30, 100);
    // label_az = lv_label_create(scr);
    // lv_obj_align(label_az, LV_ALIGN_TOP_LEFT, 30, 120);


    img1 = lv_image_create(scr);
    // lv_image_set_src(img1, "/0:/image/img1.bin");
    load_img_to_ram(img1);
    lv_image_set_pivot(img1, 120, 120); // 設定旋轉中心為圖片中心
    lv_obj_align(img1, LV_ALIGN_CENTER, 0, 0);
    // lv_group_add_obj(g, label);
}

static void app_test_run(void *scr)
{
    // Main loop code for foreground test app
    static input_angle_t prev_angle;
    input_angle_t angle;
    input_get(&angle);
    if (angle.roll != prev_angle.roll) {
        float cos_factor = fabsf(cosf(angle.roll * 3.14159f / 180.0f));
        int32_t scale_x = (int32_t)(cos_factor * 256);

        // lv_image_set_scale_x(img1, scale_x); // 調整水平寬度
        lv_image_set_rotation(img1, (int32_t)(-angle.roll * 10.0f)); // 同時旋轉
        prev_angle = angle;
    }
}

