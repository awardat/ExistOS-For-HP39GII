# N-Queens benchmark for the HP39GII Python app (MicroPython 1.29)
# Counts all solutions of the 8x8 board (expected: 92 solutions, 2057 nodes)
# Reference: "Calculator Benchmark" (876-node formulation) -> see ref/nqueens/
# Run: F4 (Run) -> pick this file, or File menu -> Open and run

import time

N = 8
col = [0] * N
nodes = 0


def ok(r, c):
    for i in range(r):
        if col[i] == c or abs(col[i] - c) == r - i:
            return False
    return True


def solve(r):
    global nodes
    nodes += 1
    if r == N:
        return 1
    total = 0
    for c in range(N):
        if ok(r, c):
            col[r] = c
            total += solve(r + 1)
    return total


t0 = time.ticks_ms()
solutions = solve(0)
t1 = time.ticks_ms()

print("N-Queens", N)
print("solutions:", solutions)
print("nodes:", nodes)
print("time:", time.ticks_diff(t1, t0), "ms")
