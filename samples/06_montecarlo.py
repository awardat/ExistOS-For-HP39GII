# -*- coding: gbk -*-
# 蒙特卡洛法估算 pi
from random import *

n = 200
hit = 0
for i in range(n):
    x = random()
    y = random()
    if x*x + y*y <= 1:
        hit = hit + 1
print("pi ~")
print(4.0*hit/n)
