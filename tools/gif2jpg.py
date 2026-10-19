#!/usr/bin/env python3
"""Convert a GIF into a baseline JPEG sequence for TJpgDec / LVGL.

Usage: gif2jpg.py input.gif out_dir [--size 240x240] [--quality 80] [--bg 000000]
"""
import argparse
import os

from PIL import Image, ImageSequence


def main():
    p = argparse.ArgumentParser()
    p.add_argument("gif")
    p.add_argument("out_dir")
    p.add_argument("--size", default="240x240", help="output WxH, aspect ratio is kept and padded")
    p.add_argument("--quality", type=int, default=80)
    p.add_argument("--bg", default="000000", help="color for transparent pixels and padding")
    args = p.parse_args()

    w, h = (int(v) for v in args.size.lower().split("x"))
    bg = tuple(int(args.bg[i:i + 2], 16) for i in (0, 2, 4))
    os.makedirs(args.out_dir, exist_ok=True)

    delays = []
    with Image.open(args.gif) as gif:
        for i, frame in enumerate(ImageSequence.Iterator(gif)):
            delays.append(frame.info.get("duration", 100) or 100)

            rgba = frame.convert("RGBA")
            rgba.thumbnail((w, h), Image.LANCZOS)

            # JPEG has no alpha: flatten onto bg and center in the target size
            out = Image.new("RGB", (w, h), bg)
            out.paste(rgba, ((w - rgba.width) // 2, (h - rgba.height) // 2), rgba)

            # TJpgDec only decodes baseline JPEG, never progressive
            out.save(os.path.join(args.out_dir, f"{i:03d}.jpg"), "JPEG",
                     quality=args.quality, progressive=False, optimize=False, subsampling="4:2:0")

    total = sum(os.path.getsize(os.path.join(args.out_dir, f"{i:03d}.jpg")) for i in range(len(delays)))
    avg_ms = sum(delays) / len(delays)
    # The Anim app plays every frame with this fixed delay
    with open(os.path.join(args.out_dir, "delay.txt"), "w") as f:
        f.write(f"{round(avg_ms)}\n")
    print(f"{len(delays)} frames, {w}x{h}, avg delay {avg_ms:.0f} ms (~{1000 / avg_ms:.1f} fps), "
          f"total {total / 1024:.1f} KB")


if __name__ == "__main__":
    main()
