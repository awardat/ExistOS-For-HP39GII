# -*- coding: gbk -*-
# 07e 自检第四季：三个最后嫌疑（各段短小独立）
#   G = abs() 调用
#   H = 列表变量下标读写 a[x]
#   I = 多条件 and
# 看最后打印的段号反馈（07d 已排除：块大小、if/else 嵌套）

print("G start")
g1 = 0
k = 0
while k < 1000:
    k = k + 1
    g1 = abs(k - 500)
print("G ok")
print(g1)

print("H start")
a = [0,0,0,0,0]
x = 0
k = 0
while k < 1000:
    k = k + 1
    if k == 1:
        x = 0
    if k == 2:
        x = 1
    if k == 3:
        x = 2
    if k == 4:
        x = 3
    if k == 5:
        x = 4
    if k > 5:
        x = 2
    a[x] = a[x] + 1
print("H ok")
print(a)

print("I start")
s = 0
k = 0
st = 0
while k < 1000:
    k = k + 1
    if k == 1:
        st = 0
    if k == 2:
        st = 1
    if k == 3:
        st = 2
    if k > 3:
        st = 1
    if st == 1 and k > 2 and k < 900:
        s = s + 1
print("I ok")
print(s)
print("all done")
