# py05_plot.py - plotting demo for the ExistOS Python app (2026-09-23)
# Counterpart of the KhiCAS sample 05_plot.py (pure Python, no CAS needed).
#
# How to run: file menu -> "run script" (or the bottom bar "run"), pick py05_plot.py.
# The graph is shown full screen; press ON or F5 to go back to the terminal.
#
# Note: keep .py sources ASCII-only (MicroPython reads UTF-8, the LCD font is GBK).

import graph
import math

# ---- 1) line plot: y = x^2 over [-2, 2] ----
graph.clf()
graph.title("y = x^2")
graph.plot(lambda t: t * t, -2, 2, 81)
graph.show()

# ---- 2) scatter plot ----
graph.clf()
graph.title("scatter: (k, k*k)")
xs = []
ys = []
k = 0
while k < 20:
    xs.append(k)
    ys.append(k * k)
    k += 1
graph.scatter(xs, ys)
graph.show()

# ---- 3) manual view (fixed axis) ----
graph.clf()
graph.title("sin(x), fixed axis")
graph.axis(0, 6.5, -1.2, 1.2)
graph.plot(lambda t: math.sin(t), 0, 6.2832, 121)
graph.show()

print("plot demo done")
