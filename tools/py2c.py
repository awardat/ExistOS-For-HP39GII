#!/usr/bin/env python3
"""把 samples/*.py 转成 pyapp 内置模块的 C 字符串（勿手改生成物）。

用法：
    python3 tools/py2c.py samples/graph3d.py graph3d_py > System/applications/user/pyapp/graph3d_py.c

版本标记：脚本中的首个 "# <名字> vYYYY-MM-DD..." 行会被提取为 <stem>_version，
设备端用它判断是否需要安装/更新（用户保留该行则不被覆盖）。
"""
import re
import sys


def main():
    if len(sys.argv) != 3:
        sys.stderr.write(__doc__)
        return 1
    src_path, stem = sys.argv[1], sys.argv[2]
    src = open(src_path, encoding="utf-8").read()
    if not src.endswith("\n"):
        src += "\n"
    m = re.search(r"^#\s*(\S+\.py\s+v\S+?)\s", src, re.M)
    version = m.group(1) if m else stem
    out = [
        "// 由 %s 自动生成（勿手改）：python3 tools/py2c.py %s %s > %s"
        % (src_path, src_path, stem, "System/applications/user/pyapp/%s.c" % stem),
        "",
        'const char *%s_version = "%s";' % (stem, version),
        "const char *%s_source =" % stem,
    ]
    for line in src.split("\n"):
        esc = line.replace("\\", "\\\\").replace('"', '\\"')
        out.append('    "%s\\n"' % esc)
    out.append(";")
    out.append("")
    sys.stdout.write("\n".join(out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
