# Zephyr ESP32 project — just recipes
# https://just.systems
# Override variables: just board=esp32_devkitc/esp32/procpu run

set shell := ["bash", "-c"]

board := "esp32_devkitc/esp32/procpu"
baud  := "921600"
venv  := justfile_directory() / "../../.venv/bin/activate"
python := justfile_directory() / "../../.venv/bin/python"

# Show available recipes
default:
    @just --list

# Open menuconfig
config:
    source {{venv}} && west build -b {{board}} -t menuconfig .

# Build
build:
    source {{venv}} && west build -p always -b {{board}} .

# Flash the connected device
flash:
    source {{venv}} && west flash --esp-baud-rate {{baud}}

# Open serial monitor
monitor:
    source {{venv}} && west espressif monitor

# Build and flash
upload: build flash

# Build, flash, and open monitor
run: build flash monitor

# Convert a GIF into a baseline JPEG sequence, e.g. just gif2jpg anim.gif out/anim 120x120
gif2jpg gif out size="240x240" quality="80" bg="000000":
    {{python}} -I {{justfile_directory()}}/tools/gif2jpg.py "{{gif}}" "{{out}}" --size {{size}} --quality {{quality}} --bg {{bg}}

# Convert all images in a folder to baseline JPEGs for the Photos app, e.g. just img2jpg ~/Pictures out/photo
img2jpg in out size="240x240" quality="85" fit="cover":
    {{python}} -I {{justfile_directory()}}/tools/img2jpg.py "{{in}}" "{{out}}" --size {{size}} --quality {{quality}} --fit {{fit}}
