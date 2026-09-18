# -*- coding: gbk -*-
# 07d 自检第三季：区分"块大小"与"嵌套"两种假设
#   E = while 体 20 行、无嵌套（测块大小）
#   F = while 体 8 行、含嵌套 if/else（测嵌套）
# 看最后打印的段号反馈（07c 已确认：无嵌套小块 A/B/C 通过）

print("E start")
k = 0
e1 = 0
while k < 3000:
    k = k + 1
    e1 = e1 + 1
    e1 = e1 - 1
    e1 = e1 + 1
    e1 = e1 - 1
    e1 = e1 + 1
    e1 = e1 - 1
    e1 = e1 + 1
    e1 = e1 - 1
    e1 = e1 + 1
    e1 = e1 - 1
    e1 = e1 + 1
    e1 = e1 - 1
    e1 = e1 + 1
    e1 = e1 - 1
    e1 = e1 + 1
    e1 = e1 - 1
    e1 = e1 + 1
    e1 = e1 - 1
    e1 = e1 + 1
    e1 = e1 - 1
print("E ok, k =")
print(k)

print("F start")
k = 0
f1 = 0
while k < 3000:
    k = k + 1
    if k == 1:
        f1 = 1
    else:
        if k == 2:
            f1 = 2
        else:
            f1 = 3
print("F ok, k =")
print(k)
print(f1)
print("all done")
