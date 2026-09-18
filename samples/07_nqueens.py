# -*- coding: gbk -*-
# 07 号样本：N-Queens 计算器基准测试（8 皇后回溯计数）
# 参考：HP Museum "Calculator Benchmark"（Xerxes, 2007）
# 算法与 HP 基准一致：单次 = 876 步（100 次 = 87600 步）
# 计时：time(1,2,3) 返回毫秒时间戳（KhiCAS 移植扩展）
# 说明：giac Python 兼容层解释执行较慢，reps 默认 1（约 876 步）；
#       确认能跑后可改大（原文基准 100 次共 87600 步，可能需数分钟）
#       须在干净会话运行（先 restart）

caseval("bench_t0:=time(1,2,3)")

reps = 1
r = 8
s = 0
a = [0,0,0,0,0,0,0,0,0]
for rep in range(reps):
    x = 0
    st = 0
    done = 0
    guard = 0
    y = 0
    t = 0
    while done == 0 and guard < 20000:
        guard = guard + 1
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
        if st == 2 and y != 0 and t == 0: st = 3
        if st == 2 and y != 0 and t != 0 and (x - y) == abs(t): st = 3
        if st == 3: a[x] = a[x] - 1
        if st == 3 and a[x] != 0: st = 1
        if st == 3 and a[x] == 0: x = x - 1
        if st == 3 and a[x] == 0 and x != 0: st = 3
        if st == 3 and a[x] == 0 and x == 0: done = 1
    print("rep done, s =")
    print(s)
    print("guard =")
    print(guard)

caseval("bench_t1:=time(1,2,3)")
caseval("bench_ms:=bench_t1-bench_t0")

print("N-Queens 8x8")
print("steps s =")
print(s)
print("elapsed ms =")
print(caseval("bench_ms"))
print("done")
