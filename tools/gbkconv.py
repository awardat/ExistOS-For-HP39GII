#!/usr/bin/env python3
# Convert UTF-8 Chinese in C/C++ string literals to GBK \xNN escapes.
# Handles: // line comments, "..." literals, \x hex-greedy trap.
import re, sys

def gbk_escape(ch):
    b = ch.encode('gbk')
    return ''.join('\\x%02x' % x for x in b)

def process_line(line):
    out = []
    i = 0
    n = len(line)
    in_str = False
    while i < n:
        c = line[i]
        if in_str:
            if c == '\\':
                out.append(c)
                if i + 1 < n:
                    out.append(line[i + 1])
                    i += 2
                    continue
                i += 1
                continue
            if c == '"':
                in_str = False
                out.append(c)
                i += 1
                continue
            # UTF-8 Chinese char (3-byte sequence)
            if ord(c) > 0x7f:
                # collect full UTF-8 sequence
                seq = c
                j = i + 1
                while j < n and ord(line[j]) > 0x7f and len(seq.encode('utf-8')) < 3:
                    seq += line[j]
                    j += 1
                esc = gbk_escape(seq)
                # hex-greedy trap: if next source char is a hex digit, need separator
                nxt = line[j] if j < n else ''
                if nxt in '0123456789abcdefABCDEF':
                    out.append(esc + '""')
                else:
                    out.append(esc)
                i = j
                continue
            out.append(c)
            i += 1
            continue
        # outside string
        if c == '/' and i + 1 < n and line[i + 1] == '/':
            out.append(line[i:])  # comment to end
            break
        if c == '"':
            in_str = True
            out.append(c)
            i += 1
            continue
        out.append(c)
        i += 1
    return ''.join(out)

def main(path, dry):
    with open(path, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    changed = 0
    newlines = []
    for ln in lines:
        nl = process_line(ln.rstrip('\n'))
        if nl != ln.rstrip('\n'):
            changed += 1
        newlines.append(nl + '\n')
    print(f"{path}: {changed} lines changed")
    if not dry:
        with open(path, 'w', encoding='utf-8') as f:
            f.writelines(newlines)

if __name__ == '__main__':
    dry = '--dry' in sys.argv
    for p in sys.argv[1:]:
        if p == '--dry':
            continue
        main(p, dry)