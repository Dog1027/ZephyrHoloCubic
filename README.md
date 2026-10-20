# ZephyrHoloCubic

用 Zephyr RTOS 和 LVGL 做的 HoloCubic 風格小螢幕裝置：ESP32 驅動一塊 240x240 的 ST7789 螢幕，畫面透過上方的分光稜鏡立方體觀看，另有 SD 卡、MPU6050 姿態感測器和一顆 WS2812 RGB LED。

操作主要透過 USB 序列埠的 shell 指令，敲一下裝置可以當成簡單的「下一個」。

## 功能

開機後是橫向輪播的 Menu，可以開啟下列 app：

| App | 說明 | 需要的 SD 卡內容 |
|---|---|---|
| Files | 瀏覽 SD 卡，資料夾排在前面 | |
| Anim | 播放 JPEG 序列動畫，循環播放 | `/anim/<名稱>/000.jpg …` |
| Photos | 相簿，可自動輪播 | `/photo/*.jpg` |
| Level | 水平儀，可設零點（重開機後保留） | |
| Dice | 骰子與抽籤，搖一搖或敲一下就擲 | `/lots.txt`（抽籤用） |
| Light | 用 RGB LED 當氣氛燈，顏色重開機後保留 | |
| Timer | 計時器，翻面朝下會暫停，離開 app 也會繼續計時 | |
| SysInfo | 運行時間、heap、SD 卡容量、各 thread 的 stack 用量 | |
| Test | 空的 app，給測試新功能用 | |

## 硬體

| 元件 | 介面 | 腳位 |
|---|---|---|
| ESP32 DevKitC | | 板子：`esp32_devkitc/esp32/procpu` |
| ST7789V 240x240 螢幕 | SPI3（MIPI DBI，DMA） | SCLK 18、MOSI 23、DC 2、RST 4、背光 5（低電位點亮） |
| SD 卡 | SPI2 | SCLK 14、MOSI 13、MISO 26、CS 15 |
| MPU6050 | I2C0，位址 0x68 | SDA 32、SCL 33（需要外部上拉電阻） |
| WS2812 RGB LED | I2S0 | DATA 27 |

腳位定義都在 [app.overlay](app.overlay)。

## 開發環境

這個資料夾是 west workspace 裡的一個 application，預期的目錄結構：

```
zephyrproject/
├── .venv/              Python 虛擬環境（west 和轉檔工具用的 Pillow 都裝在這裡）
├── zephyr/             Zephyr 4.3
├── modules/
└── applications/
    └── cube/           這個專案
```

需要：

- Zephyr 4.3 與對應的 Zephyr SDK（目前用 0.17.4）
- CMake、Ninja、gperf
- [just](https://just.systems)

## 編譯與燒錄

```bash
just build      # 編譯，輸出在 build/
just flash      # 燒錄
just monitor    # 序列埠 console（Ctrl+] 離開）
just upload     # 編譯並燒錄
just run        # 編譯、燒錄並開啟 console
just config     # menuconfig
```

換板子或燒錄速度：`just board=<board> build`、`just baud=115200 flash`。

## 操作

在 `just monitor` 裡輸入指令：

```
app list              列出所有 app 和 index
app open Photos       開啟 app（名稱不分大小寫，也可以用 index）
app close             關閉 app，回到 Menu

ui next / ui prev     下一個 / 上一個
ui ok                 選取
ui back               返回 Menu
ui tap                等同敲一下裝置
ui remote             遙控模式

tool imu_get          讀取加速度
tool led_set R G B    直接設定 LED 顏色（0-255）
```

`ui remote` 進入遙控模式後，按鍵會直接控制裝置：

| 按鍵 | 功能 |
|---|---|
| 方向鍵，或 `w` `a` `s` `d` | 上一個 / 下一個 |
| `Enter` 或空白鍵 | 選取 |
| `e` | 等同敲一下裝置 |
| `q`、`Esc`、`Backspace` 或 `b` | 關閉 app，返回 Menu |
| `Ctrl+Q` 或 `Ctrl+C` | 離開遙控模式 |

**敲一下裝置**在 app 裡預設等於 `ui next`，在 Menu 不會有反應（避免誤觸換掉選擇）。部分 app 會改成更合適的用途：Photos 換下一張、Dice 擲骰、Level 設零點、Timer 在計時中暫停或繼續。

## 準備 SD 卡內容

SD 卡要格式化成 FAT32（不支援 exFAT）。韌體不會自動格式化，必要時在 shell 用 `fs mkfs`。

圖片與動畫必須是 **baseline JPEG**，尺寸不超過 240x240。一般軟體存出來的 JPEG 常是 progressive，板子會解不出來，請用附的轉檔工具：

```bash
# 照片：整個資料夾轉成 240x240，裁切填滿（contain 則是保留完整畫面加黑邊）
just img2jpg ~/Pictures out/photo
just img2jpg ~/Pictures out/photo 240x240 85 contain

# GIF 轉成動畫：輸出 000.jpg、001.jpg … 和記錄每格時間的 delay.txt
just gif2jpg cat.gif out/cat
just gif2jpg cat.gif out/cat 120x120
```

轉好後依照下面的結構複製到 SD 卡：

```
/photo/                 Photos 的照片
/anim/cat/              每個動畫一個資料夾
    000.jpg
    001.jpg
    delay.txt           每格毫秒數（選用，預設 100）
/lots.txt               抽籤清單，一行一項（字型只支援英數字）
```

動畫的編號不能有缺號，播放時讀不到下一格就會回到第一格。尺寸越小播放越順，120x120 的解碼量只有 240x240 的四分之一。

## 新增 app

1. 在 `src/apps/fg/` 新增檔案，例如 `app_clock.c`：

   ```c
   #include "fg_app.h"
   #include "ui.h"

   APP_FG_REGISTER(Clock, LV_SYMBOL_BELL, app_clock_init, app_clock_run, app_clock_exit, 20);

   static lv_obj_t *label;

   static void app_clock_init(void *scr)
   {
       ui_screen_init(scr);
       label = lv_label_create(scr);
       lv_obj_center(label);
   }

   static void app_clock_run(void *scr)
   {
       lv_label_set_text_fmt(label, "%u", k_uptime_get_32() / 1000);
   }

   static void app_clock_exit(void *scr)
   {
   }
   ```

2. 在 [src/apps/CMakeLists.txt](src/apps/CMakeLists.txt) 加上 `target_sources(app PRIVATE fg/app_clock.c)`。

規則：

- **註冊參數**：名稱、Menu 圖示（`LV_SYMBOL_*`）、三個 callback、priority。priority 決定在 Menu 的順序，不能是 0（保留給 Menu），每個 app 用不同的值。
- **`init`**：每次開啟 app 都會呼叫，畫面已經清空，物件要在這裡重新建立。
- **`run`**：每一幀呼叫一次（間隔不固定，最長約 50 ms），不能長時間阻塞。
- **`exit`**：離開時呼叫，用來釋放自己配置的資源；畫面上的物件由框架刪除。
- **只能在這三個 callback 裡呼叫 LVGL**，所有 LVGL 操作都在同一個 thread。
- **可選取的物件**要加進 `lv_group_get_default()`，並移除 `LV_OBJ_FLAG_SCROLLABLE`，選取時會收到 `LV_EVENT_CLICKED`。`ui_list_item()` 和 `ui_tap_area()` 已經處理好這些。
- **想自己處理 next / prev / ok / 敲擊**（例如換照片）：在 `init` 呼叫 `nav_set_handler()`，回傳 `true` 表示已處理；app 關閉時框架會自動清掉。

## 專案結構

```
app.overlay             硬體與腳位（devicetree）
prj.conf                Kconfig 設定
src/
  app_thread.c          LVGL 主迴圈，切換背景 / 前景 app
  app_mgr.c             app 切換事件佇列
  nav.c                 導航指令佇列（UART、敲擊）
  tool.c                shell 指令：app、ui、tool
  input.c               MPU6050 姿態角與敲擊偵測
  gui.c                 螢幕與背光初始化
  ui.c                  共用的 LVGL 元件（標題、清單）
  jpeg.c                JPEG 解碼後直接寫到螢幕
  led.c                 RGB LED 與基礎顏色
  prefs.c               斷電保存的設定（settings / NVS）
  file.c                SD 卡掛載
  lib/tjpgd/            TJpgDec（專案內的設定，見 tjpgdcnf.h）
  apps/bg/app_menu.c    Menu
  apps/fg/              各個 app
tools/                  圖片與動畫轉檔工具
```

## 注意事項

這些是開發過程中踩到的問題，修改相關設定前請先看：

- **畫面鏡像**：`app.overlay` 的 `mdac = <0x40>` 是左右鏡像，給透過稜鏡觀看用。如果看到的畫面上下顛倒，改用 `0x80` 並把 `y-offset` 設成 `80`；直接看螢幕則用 `0x00`。傾斜方向相反時用 `CONFIG_INPUT_ROLL_INVERT` / `CONFIG_INPUT_PITCH_INVERT`。
- **只能一條 SPI 開 DMA**：兩條 SPI 共用 ESP32 的 DMA 時脈，Zephyr 驅動會讓第二條初始化失敗（log 出現 "Could not enable DMA clock"），所以只有螢幕的 SPI3 開 DMA。
- **SPI 速度**：SD 卡的 MISO 在 GPIO 26，要經過 GPIO 矩陣，超過 20 MHz 讀不到正確資料。螢幕目前 30 MHz。
- **I2C 維持 100 kHz**：腳位沒有開內建上拉，400 kHz 時 MPU6050 會初始化失敗。
- **不要在 `SYS_INIT` 裡繪圖**：開了 LVGL flush thread 後，它要等所有 `SYS_INIT` 結束才會啟動，在那之前繪圖會讓開機卡死。第一次繪圖在 `app_thread` 裡的 `gui_start()`。
- **LVGL 記憶體**：Menu 會一直留在背景，加上前景 app 和切換動畫，32 KB 不夠，目前設為 64 KB。可以用 SysInfo 看實際用量。
- **字型**：內建的 Montserrat 不含中文。

## 授權

`src/lib/tjpgd/` 是 ChaN 的 TJpgDec，授權見 [src/lib/tjpgd/LICENSE.txt](src/lib/tjpgd/LICENSE.txt)。
