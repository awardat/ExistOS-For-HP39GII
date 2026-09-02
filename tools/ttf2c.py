#!/usr/bin/env python3
"""TTF -> 1bpp C 位图字库（ExistOS 格式：每字符每行 ceil(宽/8) 字节，MSB 优先）
用法: python3 ttf2c.py <font.ttf> <px> [--name NAME] [--glyphs ASCII范围]
输出: tools/ 下生成 {name}.c，数组可直接替换 VGA_Ascii_* 使用（draw_char_ascii 加对应 case）
"""
import sys, os
from PIL import Image, ImageDraw, ImageFont

def render(font_path, px, chars):
    """返回 (宽, 高, {ch: rows_byte_list})——等宽：取最大 advance 宽度"""
    f = ImageFont.truetype(font_path, px)
    # 度量每字符宽度（advance），取整字宽（等宽字体一致，非等宽取最大）
    widths = {}
    tmp = Image.new("1", (8, 8))
    d = ImageDraw.Draw(tmp)
    for ch in chars:
        bbox = d.textbbox((0, 0), ch, font=f)
        widths[ch] = bbox[2] - bbox[0]
    w = max(widths.values()) + 1  # 等宽宽 = 最大字形宽 + 1 间距
    # 渲染
    glyphs = {}
    for ch in chars:
        bbox = d.textbbox((0, 0), ch, font=f)
        gw, gh = bbox[2] - bbox[0], bbox[3] - bbox[1]
        img = Image.new("1", (w, px + 4), 0)
        dd = ImageDraw.Draw(img)
        dd.text((0, -bbox[1]), ch, font=f, fill=1)
        rows = []
        for y in range(px):
            byte = 0
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
    chars = [chr(c) for c in range(32, 127)]
    w, h, glyphs = render(font_path, px, chars)
    nbytes = (w + 7) // 8
    out = []
    out.append("// 由 tools/ttf2c.py 生成：%s %dpx（等宽 %dpx/字符，%d 字节/行）" % (os.path.basename(font_path), px, w, nbytes))
    out.append("const unsigned char %s[] = {" % name)
    for ch in chars:
        out.append("    // '%c' (%d)" % (ch, ord(ch)))
        for row in glyphs[ch]:
            out.append("    0x%02X," % row[0] if nbytes == 1 else "    " + ", ".join("0x%02X" % b for b in row) + ",")
    out.append("};")
    out.append("// 字符宽 %d（font_w）、高 %d（font_h）；索引起点 = (ch - ' ') * %d" % (w, px, w * nbytes))
    c = "\n".join(out) + "\n"
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), name + ".c")
    open(path, "w").write(c)
    print("生成 %s：宽 %d 高 %d 字符 %d 字节/行=%d 总 %d 字节" % (path, w, px, len(chars), nbytes, len(chars) * px * nbytes))

if __name__ == "__main__":
    main()