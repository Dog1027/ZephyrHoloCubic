#!/usr/bin/env python3
"""Convert every image in a folder into baseline JPEGs for the Photos app (TJpgDec).

Usage: img2jpg.py in_dir out_dir [--size 240x240] [--quality 85] [--bg 000000] [--fit cover|contain]
"""
import argparse
import os

from PIL import Image, ImageOps

EXTS = {".jpg", ".jpeg", ".png", ".bmp", ".gif", ".webp"}


def convert(src, w, h, bg, fit):
    with Image.open(src) as img:
        img.seek(0)  # GIF / animated WebP: first frame only
        img = ImageOps.exif_transpose(img)
        rgba = img.convert("RGBA")

    if fit == "cover":
        # Fill the whole target and center-crop the overflow
        rgba = ImageOps.fit(rgba, (w, h), Image.LANCZOS, centering=(0.5, 0.5))
    else:
        # Keep everything visible, letterbox with bg
        rgba = ImageOps.contain(rgba, (w, h), Image.LANCZOS)

    # JPEG has no alpha: flatten onto bg and center in the target size
    out = Image.new("RGB", (w, h), bg)
    out.paste(rgba, ((w - rgba.width) // 2, (h - rgba.height) // 2), rgba)
    return out


def main():
    p = argparse.ArgumentParser()
    p.add_argument("in_dir")
    p.add_argument("out_dir")
    p.add_argument("--size", default="240x240", help="output WxH")
    p.add_argument("--quality", type=int, default=85)
    p.add_argument("--bg", default="000000", help="color for transparent pixels and letterbox padding")
    p.add_argument("--fit", choices=("cover", "contain"), default="cover",
                   help="cover: fill and center-crop, contain: letterbox")
    args = p.parse_args()

    w, h = (int(v) for v in args.size.lower().split("x"))
    bg = tuple(int(args.bg[i:i + 2], 16) for i in (0, 2, 4))
    os.makedirs(args.out_dir, exist_ok=True)

    count, failed, total = 0, 0, 0
    for name in sorted(os.listdir(args.in_dir)):
        src = os.path.join(args.in_dir, name)
        stem, ext = os.path.splitext(name)
        if not os.path.isfile(src) or ext.lower() not in EXTS:
            continue
        try:
            out = convert(src, w, h, bg, args.fit)
        except Exception as e:  # skip unreadable files, keep going
            print(f"skip {name}: {e}")
            failed += 1
            continue

        dst = os.path.join(args.out_dir, f"{stem}.jpg")
        # TJpgDec only decodes baseline JPEG, never progressive
        out.save(dst, "JPEG", quality=args.quality, progressive=False, optimize=False, subsampling="4:2:0")
        total += os.path.getsize(dst)
        count += 1

    print(f"{count} images, {w}x{h} {args.fit}, {failed} skipped, total {total / 1024:.1f} KB")


if __name__ == "__main__":
    main()
