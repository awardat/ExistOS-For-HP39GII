# -*- coding: gbk -*-
# 07 号样本：N-Queens 计算器基准测试（8 皇后回溯计数）
# 参考：HP Museum "Calculator Benchmark"（Xerxes, 2007）
# 算法与基准一致：单次 876 步（原文 100 次 = 87600 步）
# 计时：time(1,2,3) => 毫秒时间戳
# 结构：单层 while + 20 行紧凑体（规避 giac Python 兼容层对大块的解析问题）
# 注意：需在干净会话运行（先 restart）

caseval("bench_t0:=time(1,2,3)")

r = 8
s = 0
a = [0,0,0,0,0,0,0,0,0]
x = 0
st = 0
done = 0
guard = 0
y = 0
t = 0
while done == 0:
    guard = guard + 1
    if guard > 20000: done = 1
    if st == 0 and x == r: done = 1
    if st == 0 and x != r:
        x = x + 1
        a[x] = r
        st = 1
    if st == 1:
        s = s + 1
        y = x
        st = 2
    if st == 2: y = y - 1
    if st == 2 and y == 0: st = 0
    if st == 2 and y != 0: t = a[x] - a[y]
    if st == 2 and y != 0 and (t == 0 or (x - y) == abs(t)): st = 3
    if st == 3: a[x] = a[x] - 1
    if st == 3 and a[x] != 0: st = 1
    if st == 3 and a[x] == 0: x = x - 1
    if st == 3 and a[x] == 0 and x != 0: st = 3
    if st == 3 and a[x] == 0 and x == 0: done = 1

caseval("bench_t1:=time(1,2,3)")
print("N-Queens 8x8")
print("steps s =")
print(s)
print("guard =")
print(guard)
print("elapsed ms =")
print(caseval("bench_t1-bench_t0"))
print("done")
