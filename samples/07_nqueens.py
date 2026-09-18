# -*- coding: gbk -*-
# 07 号样本：N-Queens 计算器基准测试（8 皇后回溯计数）
# 参考：HP Museum "Calculator Benchmark"（Xerxes, 2007）
#   www.hpmuseum.org/cgi-bin/articles.cgi?read=700
# 算法与原基准程序一致：单次 qbench = 876 步
# 计时：time(1,2,3) 返回毫秒时间戳（KhiCAS 移植扩展）
# 结构：零嵌套（所有判断为顶层 if + and 条件组合）——适配 giac Python 兼容层
# 注意：reps 默认 10（原文基准 100）；须在干净会话运行（先 restart）

caseval("bench_t0:=time(1,2,3)")

reps = 10
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
    while done == 0:
        guard = guard + 1
        if guard > 20000:
            done = 1
            print("guard stop")
        if st == 0 and x == r:
            done = 1
        if st == 0 and x != r:
            x = x + 1
            a[x] = r
            st = 1
        if st == 1:
            s = s + 1
            y = x
            st = 2
        if st == 2:
            y = y - 1
        if st == 2 and y == 0:
            st = 0
        if st == 2 and y != 0:
            t = a[x] - a[y]
        if st == 2 and y != 0 and t == 0:
            st = 3
        if st == 2 and y != 0 and t != 0 and (x - y) == abs(t):
            st = 3
        if st == 3:
            a[x] = a[x] - 1
        if st == 3 and a[x] != 0:
            st = 1
        if st == 3 and a[x] == 0:
            x = x - 1
        if st == 3 and a[x] == 0 and x != 0:
            st = 3
        if st == 3 and a[x] == 0 and x == 0:
            done = 1
    print("rep done, s =")
    print(s)

caseval("bench_t1:=time(1,2,3)")
caseval("bench_ms:=bench_t1-bench_t0")

print("N-Queens 8x8")
print("steps s =")
print(s)
print("elapsed ms =")
print(caseval("bench_ms"))
print("done")
