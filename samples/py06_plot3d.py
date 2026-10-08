# py06_plot3d.py - 3D plotting demo for the ExistOS Python app (2026-10-08)
#
# How to run: file menu -> "run script" (or the bottom bar "run"), pick py06_plot3d.py.
# In the 3D view: left/right rotate, up/down tilt, ON or F5 back to the terminal.
#
# Note: keep .py sources ASCII-only (MicroPython reads UTF-8, the LCD font is GBK).

import graph3d
import math

# ---- 1) surface: z = sin(x) * cos(y) ----
graph3d.clf()
graph3d.title("z = sin(x)*cos(y)")
graph3d.view(45, 30)
graph3d.surface(lambda x, y: math.sin(x) * math.cos(y), -3, 3, -3, 3, 14)
graph3d.show()

# ---- 2) saddle: z = (x^2 - y^2) / 4 ----
graph3d.clf()
graph3d.title("z = (x^2-y^2)/4")
graph3d.view(35, 25)
graph3d.surface(lambda x, y: (x * x - y * y) * 0.25, -2, 2, -2, 2, 12)
graph3d.show()

# ---- 3) parametric curve: helix (cos t, sin t, t/5) ----
graph3d.clf()
graph3d.title("helix")
xs = []
ys = []
zs = []
k = 0
while k <= 120:
    t = k * 0.15
    xs.append(math.cos(t))
    ys.append(math.sin(t))
    zs.append(t * 0.2)
    k += 1
graph3d.plot(xs, ys, zs)
graph3d.view(50, 20)
graph3d.show()
