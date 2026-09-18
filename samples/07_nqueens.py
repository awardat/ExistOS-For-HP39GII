# -*- coding: gbk -*-
# 07 号样本：N-Queens 计算器基准测试（8 皇后回溯计数）
# 参考：HP Museum "Calculator Benchmark"（Xerxes, 2007）
#   www.hpmuseum.org/cgi-bin/articles.cgi?read=700
# 算法与原基准程序一致：单次 qbench = 876 步，100 次 = 87600 步
# 计时：time(1,2,3) 返回毫秒时间戳（KhiCAS 移植扩展，等价 C 的 rtc_get_tick_ms）
# 注意：reps 默认 10（原文基准为 100）；须在干净会话运行（先 restart）
# 诊断：每个 rep 打印一次 s；while 内置保护计数（超限自动停止）

# ---- 计时开始 ----
caseval("bench_t0:=time(1,2,3)")

# ---- 基准：8 皇后回溯迭代（Xerxes 算法，状态机等价移植）----
reps = 10
r = 8
s = 0
a = [0,0,0,0,0,0,0,0,0]
for rep in range(reps):
    x = 0
    st = 0
    guard = 0
    while 1:
        guard = guard + 1
        if guard > 20000:
            print("guard stop, st =")
            print(st)
            break
        if st == 0:
            if x == r:
                break
            x = x + 1
            a[x] = r
            st = 1
        elif st == 1:
            s = s + 1
            y = x
            st = 2
        elif st == 2:
            y = y - 1
            if y == 0:
                st = 0
            else:
                t = a[x] - a[y]
                if t == 0:
                    st = 3
                elif (x - y) != abs(t):
                    st = 2
                else:
                    st = 3
        elif st == 3:
            a[x] = a[x] - 1
            if a[x] != 0:
                st = 1
            else:
                x = x - 1
                if x == 0:
                    break
    print("rep done, s =")
    print(s)

# ---- 计时结束 ----
caseval("bench_t1:=time(1,2,3)")
caseval("bench_ms:=bench_t1-bench_t0")

# ---- 结果输出 ----
print("N-Queens 8x8")
print("steps s =")
print(s)
print("elapsed ms =")
print(caseval("bench_ms"))
print("done")
