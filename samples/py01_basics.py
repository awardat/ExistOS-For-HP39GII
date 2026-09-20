# Python basics for the HP39GII Python app (MicroPython 1.29)
# Run: F4 (Run) -> pick this file, or File menu -> Open and run

print("Hello, Python!")

# arithmetic
print(2 + 3 * 4)
print(7 / 2)
print(7 // 2, 7 % 2, 2 ** 10)

# variables and strings
name = "HP39GII"
print("Hello, " + name)
print("len =", len(name))

# lists and loops
squares = []
for i in range(1, 6):
    squares.append(i * i)
print("squares:", squares)
print("sum:", sum(squares))
print("max:", max(squares))

# conditions
x = 42
if x > 100:
    print("big")
elif x > 10:
    print("medium")
else:
    print("small")

# while loop
n = 10
total = 0
while n > 0:
    total += n
    n -= 1
print("1+...+10 =", total)

# functions
def fib(k):
    a, b = 0, 1
    for _ in range(k):
        a, b = b, a + b
    return a

print("fib(20) =", fib(20))

# dictionaries
d = {"a": 1, "b": 2}
d["c"] = 3
print("dict:", d)
print("keys:", list(d.keys()))

# strings
s = "MicroPython"
print(s.upper(), s[0:5], s[5:])  # note: MicroPython does not support negative slice steps

# timing
import time
t0 = time.ticks_ms()
t = 0
for i in range(1000):
    t += i * i
t1 = time.ticks_ms()
print("loop 1000 took", time.ticks_diff(t1, t0), "ms")

print("Done.")
