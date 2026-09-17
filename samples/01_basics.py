# -*- coding: gbk -*-
# 基础示例：变量、循环、条件、函数
# 打开：KhiCAS -> 文件(Fich) -> 打开脚本 -> 选择本文件（或主界面 APPS 键）

print("Hello, HP39GII!")

# 变量与运算
a = 3
b = 4
print("3^2+4^2 =")
print(a*a + b*b)

# 循环：1 到 10 求和
s = 0
for i in range(1, 11):
    s = s + i
print("1+2+...+10 =")
print(s)

# 条件与函数
def sign(x):
    if x > 0:
        return 1
    elif x < 0:
        return -1
    else:
        return 0

print("sign(-5) =")
print(sign(-5))
print("sign(7) =")
print(sign(7))
