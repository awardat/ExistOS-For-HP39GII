#!/usr/bin/env python3
"""TTF -> 1bpp C 位图字库（ExistOS 格式：每字符每行 ceil(宽/8) 字节，MSB 优先）
用法: python3 ttf2c.py <font.ttf> <px> [--name NAME] [--pad N]
  --pad 0/1：字符定宽 = 最大字形宽 + pad（等宽字体通常 1；非等宽如 Fira Sans 用 0 或 1）
输出: tools/ 下生成 {name}.c，复制到 System/graphics/ 并加 aux 扫描即编译

唯一权威源（2026-09-03 标注）：本脚本 + tools/ 的生成物。修改字体必须：
  1) 在本目录重新生成（勿手改 System/graphics/ 副本）
  2) 复制到 System/graphics/ 覆盖同文件
  3) 同步 UICore.h 中的字库引用（symbol 名/规格注释）
当前在用（RPN39/系统）：FiraCodeAscii24（24px 等宽）、FiraSansAscii20/32（20/32px）
tools/ 其余字库（FiraAscii20/32、HackAscii20 等）为历史尝试废弃物，保留供对比，勿用于新代码。
"""
import sys, os
from PIL import Image, ImageDraw, ImageFont

def render(font_path, px, chars, pad, adv=None):
    f = ImageFont.truetype(font_path, px)
    tmp = Image.new("1", (8, 8))
    d = ImageDraw.Draw(tmp)
    widths = {ch: d.textbbox((0, 0), ch, font=f)[2] for ch in chars}
    if adv is not None:
        w = adv
    else:
        w = max(widths.values()) + pad  # 字符定宽
    glyphs = {}
    for ch in chars:
        bbox = d.textbbox((0, 0), ch, font=f)
        img = Image.new("1", (w, px + 4), 0)
        dd = ImageDraw.Draw(img)
        dd.text((0, -bbox[1]), ch, font=f, fill=1)
        rows = []
        for y in range(px):
            row = []
            for x in range(0, w, 8):
                b = 0
                for k in range(8):
                    if x + k < w and img.getpixel((x + k, y)):
                        b |= 1 << (7 - k)
                row.append(b)
            rows.append(bytes(row))
        glyphs[ch] = rows
    return w, px, glyphs

def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    font_path, px = sys.argv[1], int(sys.argv[2])
    name = "FontAscii%d" % px
    if "--name" in sys.argv:
        name = sys.argv[sys.argv.index("--name") + 1]
    pad = 1
    if "--pad" in sys.argv:
        pad = int(sys.argv[sys.argv.index("--pad") + 1])
    adv = None
    if "--advance" in sys.argv:  # 非等宽字体：按数字定宽（字母 M/W 可裁，仅数字/符号场景）
        adv = int(sys.argv[sys.argv.index("--advance") + 1])
    chars = [chr(c) for c in range(32, 127)]
    w, h, glyphs = render(font_path, px, chars, pad, adv)
    nbytes = (w + 7) // 8
    out = []
    out.append("// 由 tools/ttf2c.py 生成：%s %dpx（定宽 %dpx/字符，%d 字节/行）" % (os.path.basename(font_path), px, w, nbytes))
    out.append("const unsigned char %s[] = {" % name)
    for ch in chars:
        out.append("    // '%c' (%d)" % (ch, ord(ch)))
        for row in glyphs[ch]:
            out.append("    " + ", ".join("0x%02X" % b for b in row) + ",")
    out.append("};")
    out.append("// 字符宽 %d（font_w）、高 %d（font_h）；索引起点 = (ch - ' ') * %d" % (w, px, w * nbytes))
    c = "\n".join(out) + "\n"
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), name + ".c")
    open(path, "w").write(c)
    print("生成 %s：宽 %d 高 %d 字符 %d 字节/行=%d 总 %d 字节" % (path, w, px, len(chars), nbytes, len(chars) * px * nbytes))

if __name__ == "__main__":
    main()