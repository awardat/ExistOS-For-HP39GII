# -*- coding: gbk -*-
# 列表与数列：斐波那契、平方数
fib = [1, 1]
for k in range(2, 12):
    fib.append(fib[k-1] + fib[k-2])
print("fibonacci:")
print(fib)

sq = []
for k in range(1, 11):
    sq.append(k*k)
print("squares:")
print(sq)

# 手写求和
total = 0
for v in sq:
    total = total + v
print("sum of squares =")
print(total)

# 手写最大值
m = sq[0]
for v in sq:
    if v > m:
        m = v
print("max =")
print(m)
