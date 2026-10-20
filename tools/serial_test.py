#!/usr/bin/env python3
"""Smoke test over the shell UART: reset the cube, open and close every app, report problems.

Usage: serial_test.py PORT LOG
Close any serial monitor first, the port can only be opened once.
"""
import re
import sys
import time

import serial

PORT = sys.argv[1]
LOG = sys.argv[2]

ser = serial.Serial()
ser.port = PORT
ser.baudrate = 115200
ser.timeout = 0.05
# CP2102 DTR/RTS drive the ESP32 boot pins, keep both released when opening
ser.dtr = False
ser.rts = False
ser.open()

out = open(LOG, "w", encoding="utf-8", errors="replace")


def read_for(seconds):
    end = time.time() + seconds
    buf = b""
    while time.time() < end:
        buf += ser.read(4096)
    text = buf.decode("utf-8", errors="replace")
    out.write(text)
    out.flush()
    return text


def cmd(line, wait):
    out.write(f"\n===== >>> {line}\n")
    # Sending a whole line at once makes the shell duplicate / drop characters, pace it
    for ch in line + "\r":
        ser.write(ch.encode())
        time.sleep(0.01)
    return read_for(wait)


# Reset through EN (RTS) to capture the whole boot
out.write("===== RESET\n")
ser.rts = True
time.sleep(0.1)
ser.rts = False
read_for(6)

cmd("", 0.5)
cmd("app list", 1)

# (app, extra commands while it is open); avoid OK where it saves settings (Level, Light) or starts Timer
plan = [
    ("Files", ["ui next", "ui next", "ui ok", "ui prev"]),
    ("Anim", ["ui next", "ui ok"]),
    ("Photos", ["ui next", "ui tap", "ui prev"]),
    ("Level", ["ui next", "ui prev"]),
    ("Dice", ["ui tap", "ui next", "ui ok", "ui prev"]),
    ("Light", ["ui next", "ui next", "ui prev"]),
    ("Timer", ["ui next", "ui next"]),
    ("SysInfo", ["ui next", "ui next", "ui next"]),
    ("Test", ["ui tap"]),
]
for app, extra in plan:
    cmd(f"app open {app}", 2.5)
    for c in extra:
        cmd(c, 1.0)
    cmd("app close", 1.5)

# Menu navigation and opening an app with OK, tap must not move the menu
cmd("ui next", 0.8)
cmd("ui tap", 0.8)
cmd("ui ok", 2.5)
cmd("ui back", 1.5)

cmd("tool imu_get", 1)
cmd("kernel thread stacks", 2)
read_for(1)
out.close()
ser.close()

# Summary
text = re.sub(r"\x1b\[[0-9;]*[A-Za-z]", "", open(LOG, encoding="utf-8", errors="replace").read())
problems = [l for l in text.splitlines()
            if re.search(r"<err>|<wrn>|FATAL|Assert|Halting|No memory|command not found", l)]
starts = text.count("Starting foreground application")
ends = text.count("Ending foreground application")
print(f"Log: {LOG}")
print(f"Apps opened / closed: {starts} / {ends} (expected {len(plan) + 1} / {len(plan) + 1})")
print(f"Problems: {len(problems)}")
for l in problems:
    print("  " + l.strip())
print("Stacks:")
for l in text.splitlines():
    if "usage" in l and "real size" in l:
        name = l.split()[1]
        print(f"  {name:20} {l.split('usage')[1].strip()}")
sys.exit(1 if problems or starts != len(plan) + 1 or ends != starts else 0)
