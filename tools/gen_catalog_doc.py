#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""从 KhiCAS 命令目录（catalog）生成中文内置函数参考文档。
用法：python3 tools/gen_catalog_doc.py Libs/src/khicas/kdisplay.cc docs/KhiCAS-functions.md
"""
import re, sys
from collections import Counter

def body_of(src, name):
    i = src.index(name); j = src.index('{', i)
    depth = 0
    for k in range(j, len(src)):
        if src[k] == '{': depth += 1
        elif src[k] == '}':
            depth -= 1
            if depth == 0: return src[j + 1:k]

def strip_comments(s):
    out = []; i = 0; instr = False; esc = False
    while i < len(s):
        ch = s[i]
        if instr:
            out.append(ch)
            if esc: esc = False
            elif ch == '\\': esc = True
            elif ch == '"': instr = False
            i += 1; continue
        if ch == '"':
            instr = True; out.append(ch); i += 1; continue
        if ch == '/' and i + 1 < len(s) and s[i + 1] == '/':
            while i < len(s) and s[i] != '\n': i += 1
            continue
        if ch == '/' and i + 1 < len(s) and s[i + 1] == '*':
            i += 2
            while i + 1 < len(s) and not (s[i] == '*' and s[i + 1] == '/'): i += 1
            i += 2; continue
        out.append(ch); i += 1
    return ''.join(out)

def dec(s):
    out = bytearray(); i = 0
    while i < len(s):
        c = s[i]
        if c == '"': i += 1; continue
        if c == '\\':
            e = s[i + 1] if i + 1 < len(s) else ''
            if e == 'x':
                j = i + 2; hx = ''
                while j < len(s) and s[j] in '0123456789abcdefABCDEF': hx += s[j]; j += 1
                out.append(int(hx, 16) & 0xFF if hx else 0); i = j
            elif e == 'n': out.append(0x0A); i += 2
            elif e == 't': out.append(0x09); i += 2
            elif e in '\\"': out.append(ord(e)); i += 2
            else: out.append(ord(e)); i += 2
        else:
            out += c.encode('utf-8'); i += 1
    try: return out.decode('gbk')
    except Exception: return out.decode('latin-1')

def split_top(s):
    parts = []; buf = ''; instr = False; esc = False
    for ch in s:
        if instr:
            buf += ch
            if esc: esc = False
            elif ch == '\\': esc = True
            elif ch == '"': instr = False
        else:
            if ch == '"': instr = True; buf += ch
            elif ch == ',': parts.append(buf.strip()); buf = ''
            else: buf += ch
    if buf.strip(): parts.append(buf.strip())
    return parts

def parse_entries(body):
    items = []; cur = None; instr = False; esc = False
    for ch in body:
        if instr:
            cur += ch
            if esc: esc = False
            elif ch == '\\': esc = True
            elif ch == '"': instr = False
            continue
        if ch == '"': instr = True; cur = (cur or '') + ch; continue
        if ch == '{':
            cur = '' if cur is None else cur + '{'; continue
        if ch == '}':
            if cur is not None: items.append(cur); cur = None
            continue
        if cur is not None: cur += ch
    return [split_top(i) for i in items]

def field_val(x):
    x = x.strip()
    if x == '0': return None
    if x.startswith('"'): return dec(x)
    return x

def main():
    srcfile, outfile = sys.argv[1], sys.argv[2]
    src = open(srcfile, encoding='utf-8', errors='replace').read()

    zh = [[field_val(f) for f in e] for e in parse_entries(strip_comments(body_of(src, 'completeCatzh[]')))]
    zhn = [dec(x) for x in split_top(strip_comments(body_of(src, 'completeCatZhName[]')))]

    cat_names = {}
    for m in re.finditer(r'menuitems\[CAT_CATEGORY_(\w+)\]\.text = \(char\*\)\(\(lang\)\?"((?:[^"\\]|\\.)*)"', src):
        cat_names[m.group(1)] = dec('"' + m.group(2) + '"')
    ids = {m.group(1): int(m.group(2)) for m in re.finditer(r'#define CAT_CATEGORY_(\w+) (\d+)', src)}
    order = [ids[k] for k, _ in sorted(ids.items(), key=lambda kv: kv[1])]

    def cats_of(expr):
        if not expr: return [], False
        xo = 'XCAS_ONLY' in expr
        out = []
        for m in re.finditer(r'CAT_CATEGORY_(\w+)(?:\s*<<\s*(\d+))?', expr):
            base = ids.get(m.group(1), 0)
            shift = int(m.group(2)) // 8 if m.group(2) else 0
            out.append(base + shift)
        return out, xo

    def clean(s):
        return (s or '').replace('\n', ' ').replace('  ', ' ').strip()

    entries = []
    for idx, e in enumerate(zh):
        name = e[0] if len(e) > 0 and e[0] else ''
        insert = e[1] if len(e) > 1 else None
        desc = e[2] if len(e) > 2 else None
        ex = e[3] if len(e) > 3 else None
        cats, xo = cats_of(e[5] if len(e) > 5 else None)
        display = zhn[idx] if idx < len(zhn) else name
        if not insert or insert == name: insert = name
        if ex and ex.startswith('#'): ex = ex[1:].strip()
        entries.append(dict(name=name, display=clean(display), insert=clean(insert),
                            desc=clean(desc), ex=clean(ex), cats=cats, xo=xo))

    cnt = Counter()
    for e in entries:
        for c in e['cats']: cnt[c] += 1
    total_xo = sum(1 for e in entries if e['xo'])

    L = []
    L.append('# KhiCAS 内置函数参考（HP39GII）\n')
    L.append('> **自动生成**（`tools/gen_catalog_doc.py`）——数据来源为 KhiCAS 命令目录（与设备内「命令列表」一致），请勿手工编辑。')
    L.append('> 适用版本：build 139/140。生成日期：2026-09-19。\n')
    L.append('**怎么用**')
    L.append('- 在 Console 直接输入命令（函数名区分大小写）；参数用逗号分隔')
    L.append('- **MATH/F4** 打开命令列表按分类浏览；**Shift+0** 输入前缀自动补全')
    L.append('- 菜单内 **F6 / Shift+View** 显示当前高亮项的帮助')
    L.append(f'- 标记 **`*`** 的条目 = 仅 Xcas 模式（Python 兼容模式下不可用，共 {total_xo} 条；该部分说明保留原文未汉化）\n')
    L.append('## 分类总览\n')
    def label_of(cid):
        cname = {v: k for k, v in ids.items()}.get(cid)
        label = cat_names.get(cname, str(cid))
        if '(' in label: label = label[:label.index('(')]
        return label.strip().rstrip('，,')
    active = [c for c in order if c != 0 and cnt.get(c, 0)]
    L.append('| 分类 | 条目数 |')
    L.append('|------|--------|')
    for cid in active:
        L.append(f'| {label_of(cid)} | {cnt[cid]} |')
    L.append('')
    L.append('> 分类次序与设备内命令列表一致（空分类不列出）。\n')
    for n, cid in enumerate(active, 1):
        L.append(f'## {n}. {label_of(cid)}\n')
        L.append('| 命令 | 用途 | 用法 / 参数 | 示例 |')
        L.append('|------|------|------------|------|')
        for e in entries:
            if cid not in e['cats']: continue
            star = '*' if e['xo'] else ''
            desc = e['desc'] or '—'
            esc = lambda s: s.replace('|', '\\|')
            L.append(f"| `{esc(e['display'])}`{star} | {esc(desc)} | `{esc(e['insert'])}` | {esc(e['ex']) or '—'} |")
        L.append('')
    open(outfile, 'w', encoding='utf-8').write('\n'.join(L) + '\n')
    print('entries:', len(entries), '| xcas_only:', total_xo, '| categories:', len([c for c in order if cnt.get(c,0)]))

if __name__ == '__main__':
    main()
