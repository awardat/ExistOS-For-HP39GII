# -*- coding: gbk -*-
# 07 号样本：N-Queens 计算器基准测试（8 皇后回溯计数）
# 参考：HP Museum "Calculator Benchmark"（Xerxes, 2007）
# 实现说明：循环体以 giac 原生代码经 caseval() 执行——
#   giac 原生解析器成熟且执行快，绕开 Python 兼容层对大块/复杂循环的限制；
#   外层仍是 Python 脚本（计时、取结果）。
# 算法：与原基准一致，单次 = 876 步（giac 向量 0 基，a[0] 占位，算法用 a[1..8]）

# ---- 计时开始 ----
caseval("bench_t0:=time(1,2,3)")

# ---- giac 原生代码（分片拼接为短字符串）----
code = ""
code = code + "r:=8; s:=0; a:=[0,0,0,0,0,0,0,0,0];"
code = code + "x:=0; st:=0; done:=0; guard:=0; y:=0; t:=0;"
code = code + "while(done==0 && guard<200000){"
code = code + " guard:=guard+1;"
code = code + " if(st==0 && x==r){done:=1};"
code = code + " if(st==0 && x!=r){x:=x+1; a[x]:=r; st:=1};"
code = code + " if(st==1){s:=s+1; y:=x; st:=2};"
code = code + " if(st==2){y:=y-1};"
code = code + " if(st==2 && y==0){st:=0};"
code = code + " if(st==2 && y!=0){t:=a[x]-a[y]};"
code = code + " if(st==2 && y!=0 && t==0){st:=3};"
code = code + " if(st==2 && y!=0 && t!=0 && (x-y)==abs(t)){st:=3};"
code = code + " if(st==3){a[x]:=a[x]-1};"
code = code + " if(st==3 && a[x]!=0){st:=1};"
code = code + " if(st==3 && a[x]==0){x:=x-1};"
code = code + " if(st==3 && a[x]==0 && x!=0){st:=3};"
code = code + " if(st==3 && a[x]==0 && x==0){done:=1}"
code = code + "}"
caseval(code)

# ---- 计时结束 ----
caseval("bench_t1:=time(1,2,3)")

# ---- 结果输出 ----
print("N-Queens 8x8 (giac native)")
print("steps s =")
print(caseval("s"))
print("guard =")
print(caseval("guard"))
print("elapsed ms =")
print(caseval("bench_t1-bench_t0"))
print("done")
