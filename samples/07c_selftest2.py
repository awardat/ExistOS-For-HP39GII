# -*- coding: gbk -*-
# 07c 自检第二季：列表元素读写 / 深层嵌套 break
# 看最后打印的段号反馈
#   A = 列表元素赋值 a[k-1]=...（预期 [2,4,6,8]）
#   B = elif 链 + else 内 break（预期 cnt=4）
#   C = 列表读改写 a[1]=a[1]-1（预期 [5,2,5]）
#   D = while 内嵌套 if 内 break（预期 s=876）
# 注意：不使用 i 变量（giac 内置虚数单位）

print("A start")
a = [0,0,0,0]
k = 0
while 1:
    k = k + 1
    a[k-1] = k * 2
    if k >= 4:
        break
print("A ok")
print(a)

print("B start")
st = 0
cnt = 0
while 1:
    cnt = cnt + 1
    if cnt > 100:
        print("B guard")
        break
    if st == 0:
        st = 1
    elif st == 1:
        st = 2
    elif st == 2:
        st = 3
    else:
        break
print("B ok, cnt =")
print(cnt)

print("C start")
a = [5,5,5]
k = 0
while 1:
    k = k + 1
    a[1] = a[1] - 1
    if k >= 3:
        break
print("C ok")
print(a)

print("D start")
r = 8
s = 0
a = [0,0,0,0,0,0,0,0,0]
x = 0
st = 0
guard = 0
while 1:
    guard = guard + 1
    if guard > 30000:
        print("D guard, st =")
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
    else:
        a[x] = a[x] - 1
        if a[x] != 0:
            st = 1
        else:
            x = x - 1
            if x == 0:
                break
print("D ok, s =")
print(s)
print("all done")
