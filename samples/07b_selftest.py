# -*- coding: gbk -*-
# 07b 自检：定位 Python 兼容层循环结构支持情况
# 用法：拷到 /xcas/ 运行，看最后打印的编号
#   T1 = while 1 + break
#   T2 = 列表索引赋值
#   T3 = for + while + elif 嵌套
#   T4 = 8 皇后状态机单次（预期 s = 876）
# 若卡在某段，把最后打印的段号/内容反馈

print("T1 start")
i = 0
while 1:
    i = i + 1
    if i >= 5:
        break
print("T1 ok, i =")
print(i)

print("T2 start")
a = [0,0,0]
i = 0
while 1:
    i = i + 1
    a[i-1] = i * 2
    if i >= 3:
        break
print("T2 ok")
print(a)

print("T3 start")
s = 0
for rep in range(2):
    x = 0
    while 1:
        x = x + 1
        if x > 3:
            break
        if x == 1:
            s = s + 1
        elif x == 2:
            s = s + 10
        else:
            s = s + 100
print("T3 ok, s =")
print(s)

print("T4 start")
r = 8
s = 0
a = [0,0,0,0,0,0,0,0,0]
x = 0
st = 0
guard = 0
while 1:
    guard = guard + 1
    if guard > 20000:
        print("T4 guard, st =")
        print(st)
        break
    if st == 0:
        if x == r:
            break
        x = x + 1
        a[x] = r
        st = 1
    elif st == 1:
        s = s + 1
        y = x
        st = 2
    elif st == 2:
        y = y - 1
        if y == 0:
            st = 0
        else:
            t = a[x] - a[y]
            if t == 0:
                st = 3
            elif (x - y) != abs(t):
                st = 2
            else:
                st = 3
    elif st == 3:
        a[x] = a[x] - 1
        if a[x] != 0:
            st = 1
        else:
            x = x - 1
            if x == 0:
                break
print("T4 ok, s =")
print(s)
print("all done")
