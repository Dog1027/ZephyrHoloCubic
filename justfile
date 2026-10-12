# Zephyr ESP32 project — just recipes
# https://just.systems
# Override variables: just board=esp32_devkitc/esp32/procpu run

set shell := ["bash", "-c"]

board := "esp32_devkitc/esp32/procpu"
baud  := "921600"
venv  := justfile_directory() / "../../.venv/bin/activate"

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
