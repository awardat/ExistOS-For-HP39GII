# -*- coding: gbk -*-
# 线性代数示例（linalg）
from linalg import *

A = matrix([[1, 2], [3, 4]])
B = matrix([[5, 6], [7, 8]])
print("A =")
print(A)
print("det(A) =")
print(det(A))
print("inv(A) =")
print(inv(A))
print("A+B =")
print(add(A, B))
print("A*B =")
print(mul(A, B))
print("transpose(A) =")
print(transpose(A))
