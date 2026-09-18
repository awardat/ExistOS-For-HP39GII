# -*- coding: gbk -*-
# 07 号样本：N-Queens 计算器基准测试（8 皇后回溯计数）
# 参考：HP Museum "Calculator Benchmark"（Xerxes, 2007）
# 实现：循环体为 giac 原生代码（caseval 执行）——giac 解析器成熟、执行快
# 语法：多行 + 花括号块（标准 giac 形式，块后不加分号）
# 算法：与原基准一致，单次 = 876 步（giac 向量 0 基，a[0] 占位，a[1..8] 使用）

caseval("bench_t0:=time(1,2,3)")

code = ""
code = code + "r:=8; s:=0; a:=[0,0,0,0,0,0,0,0,0];\n"
code = code + "x:=0; st:=0; done:=0; guard:=0; y:=0; t:=0;\n"
code = code + "while(done==0 && guard<200000){\n"
code = code + "guard:=guard+1;\n"
code = code + "if(st==0 && x==r){done:=1}\n"
code = code + "if(st==0 && x!=r){\nx:=x+1;\na[x]:=r;\nst:=1\n}\n"
code = code + "if(st==1){\ns:=s+1;\ny:=x;\nst:=2\n}\n"
code = code + "if(st==2){y:=y-1}\n"
code = code + "if(st==2 && y==0){st:=0}\n"
code = code + "if(st==2 && y!=0){t:=a[x]-a[y]}\n"
code = code + "if(st==2 && y!=0 && t==0){st:=3}\n"
code = code + "if(st==2 && y!=0 && t!=0 && (x-y)==abs(t)){st:=3}\n"
code = code + "if(st==3){a[x]:=a[x]-1}\n"
code = code + "if(st==3 && a[x]!=0){st:=1}\n"
code = code + "if(st==3 && a[x]==0){x:=x-1}\n"
code = code + "if(st==3 && a[x]==0 && x!=0){st:=3}\n"
code = code + "if(st==3 && a[x]==0 && x==0){done:=1}\n"
code = code + "}\n"
caseval(code)

caseval("bench_t1:=time(1,2,3)")

print("N-Queens 8x8 (giac native v2)")
print("steps s =")
print(caseval("s"))
print("guard =")
print(caseval("guard"))
print("elapsed ms =")
print(caseval("bench_t1-bench_t0"))
print("done")
