# -*- coding: gbk -*-
# 绘图示例：函数曲线与散点
from matplotl import *

clf()
x = []
y = []
for i in range(0, 41):
    t = i/10.0 - 2.0
    x.append(t)
    y.append(t*t)
plot(x, y)
show()

# 散点
clf()
x2 = []
y2 = []
for i in range(0, 20):
    x2.append(i)
    y2.append(i*i)
scatter(x2, y2)
show()
