# -*- coding: gbk -*-
# 数学示例：内建数学函数、素数、CAS 调用
from math import *

print("floor(3.7) =")
print(floor(3.7))
print("ceil(3.2) =")
print(ceil(3.2))
print("sqrt(2) =")
print(sqrt(2))

# 素数判定（试除法）
def is_prime(n):
    if n < 2:
        return 0
    d = 2
    while d*d <= n:
        if n % d == 0:
            return 0
        d = d + 1
    return 1

print("primes < 30:")
for n in range(2, 30):
    if is_prime(n):
        print(n)

# 调用 CAS 引擎（符号计算能力）
from cas import *
print("factor(x^4-1) =")
print(caseval("factor(x^4-1)"))
print("solve(x^2-5*x+6,x) =")
print(caseval("solve(x^2-5*x+6,x)"))
