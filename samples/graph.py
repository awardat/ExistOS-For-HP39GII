# graph.py v2026-09-24b - a minimal plotting module for the ExistOS Python app (2026-09-24)
#
# ASCII-only source (MicroPython reads UTF-8; the LCD font is GBK-indexed).
# API:
#   clf()                     clear the current figure
#   plot(x, y)                line plot from two sequences
#   plot(f, a, b, n=200)      line plot of a callable over [a, b]
#   scatter(x, y)             point plot
#   axis(xmin, xmax, ymin, ymax)   fix the view (disables auto scaling)
#   title(s)                  figure title (ASCII)
#   show()                    render full screen and wait for ON/F5
#
# Example:
#   import graph
#   graph.plot(lambda t: t * t, -2, 2)
#   graph.show()

import framebuf
import lcd
import math

_TOP = 14            # leave the top icon row to the system
_BAR = 12            # bottom hint line
_GRAY = 200          # unused for 1bpp, kept for clarity

_g = None


class _Graph(object):
    def __init__(self):
        self.w = lcd.width()
        self.h = lcd.height()
        self.px0 = 0
        self.py0 = _TOP
        self.pw = self.w
        self.ph = self.h - _TOP - _BAR
        self.fb = None
        self.xmin = 0.0
        self.xmax = 1.0
        self.ymin = 0.0
        self.ymax = 1.0
        self.auto = True
        self.xs = []
        self.ys = []
        self.mode = 0        # 0 = line, 1 = scatter
        self._title = ""
        self._labels = []

    # ---------- figure state ----------
    def clf(self):
        self.xs = []
        self.ys = []
        self.auto = True
        self.mode = 0
        self._title = ""
        self._labels = []

    def axis(self, xmin, xmax, ymin, ymax):
        if xmax > xmin and ymax > ymin:
            self.xmin = float(xmin)
            self.xmax = float(xmax)
            self.ymin = float(ymin)
            self.ymax = float(ymax)
            self.auto = False

    def title(self, s):
        self._title = s

    # ---------- data ----------
    def plot(self, a, b=None, c=None, d=None):
        # plot(f, lo, hi, n) or plot(xs, ys)
        if callable(a):
            lo = float(b)
            hi = float(c) if c is not None else lo + 1.0
            n = int(d) if d is not None else 200
            if n < 2:
                n = 2
            xs = []
            ys = []
            i = 0
            while i < n:
                t = lo + (hi - lo) * i / (n - 1)
                xs.append(t)
                ys.append(a(t))
                i += 1
            self._set(xs, ys, 0)
        else:
            self._set(a, b, 0)

    def scatter(self, x, y):
        self._set(x, y, 1)

    def _set(self, xs, ys, mode):
        if xs is None or ys is None or len(xs) != len(ys) or len(xs) == 0:
            raise ValueError("x/y must be non-empty sequences of equal length")
        self.xs = [float(v) for v in xs]
        self.ys = [float(v) for v in ys]
        self.mode = mode

    # ---------- scaling helpers ----------
    def _bounds(self):
        if self.auto:
            xmin = min(self.xs)
            xmax = max(self.xs)
            ymin = min(self.ys)
            ymax = max(self.ys)
            if xmax - xmin < 1e-12:
                xmin -= 0.5
                xmax += 0.5
            if ymax - ymin < 1e-12:
                ymin -= 0.5
                ymax += 0.5
            dx = (xmax - xmin) * 0.05
            dy = (ymax - ymin) * 0.10
            self.xmin = xmin - dx
            self.xmax = xmax + dx
            self.ymin = ymin - dy
            self.ymax = ymax + dy
        return self.xmin, self.xmax, self.ymin, self.ymax

    def _mx(self, xv):
        return int(self.px0 + (xv - self.xmin) * (self.pw - 1) / (self.xmax - self.xmin) + 0.5)

    def _my(self, yv):
        return int(self.py0 + self.ph - 1 - (yv - self.ymin) * (self.ph - 1) / (self.ymax - self.ymin) + 0.5)

    @staticmethod
    def _nice(lo, hi, want):
        # pick "nice" tick step (1/2/5 * 10^k) and return the tick list
        if hi <= lo:
            return []
        span = hi - lo
        raw = span / want
        mag = math.pow(10.0, math.floor(math.log10(raw)))
        for m in (1.0, 2.0, 5.0, 10.0):
            step = m * mag
            if step >= raw:
                break
        ticks = []
        t = math.ceil(lo / step) * step
        n = 0
        while t <= hi + step * 1e-6 and n < 40:
            ticks.append(t)
            t += step
            n += 1
        return ticks

    @staticmethod
    def _fmt(v):
        if abs(v) < 1e-12:
            return "0"
        a = abs(v)
        if a >= 1e5 or a < 1e-3:
            return "%.0e" % v
        if a >= 100:
            return "%.0f" % v
        if a >= 1:
            return ("%.2f" % v).rstrip("0").rstrip(".")
        return ("%.3f" % v).rstrip("0").rstrip(".")

    # ---------- drawing ----------
    def _frame(self):
        self.fb = framebuf.FrameBuffer(bytearray((self.w * self.h + 7) // 8), self.w, self.h, framebuf.MONO_HLSB)
        self.fb.fill(0)                                                       # background
        self.fb.rect(0, 0, self.w, self.h, 1)
        self.fb.line(0, _TOP - 1, self.w - 1, _TOP - 1, 1)

    def _axes(self):
        fb = self.fb
        self._labels = []
        x0y = self._my(0.0) if self.ymin < 0 < self.ymax else self.py0 + self.ph - 1
        y0x = self._mx(0.0) if self.xmin < 0 < self.xmax else self.px0
        if x0y < self.py0:
            x0y = self.py0
        if x0y > self.py0 + self.ph - 1:
            x0y = self.py0 + self.ph - 1
        fb.line(self.px0, x0y, self.px0 + self.pw - 1, x0y, 1)                # x axis
        fb.line(y0x, self.py0, y0x, self.py0 + self.ph - 1, 1)                # y axis
        # x ticks
        tx = self._nice(self.xmin, self.xmax, 4)
        for t in tx:
            px = self._mx(t)
            if px < self.px0 or px >= self.px0 + self.pw:
                continue
            fb.line(px, x0y - 2, px, x0y + 2, 1)
            s = self._fmt(t)
            self._labels.append((px - 4 * len(s), x0y - 12, s, 12))
        # y ticks
        ty = self._nice(self.ymin, self.ymax, 4)
        for t in ty:
            py = self._my(t)
            if py < self.py0 or py >= self.py0 + self.ph:
                continue
            fb.line(y0x - 2, py, y0x + 2, py, 1)
            s = self._fmt(t)
            lx = y0x + 3
            if lx + 6 * len(s) > self.w - 1:
                lx = y0x - 3 - 6 * len(s)
            self._labels.append((lx, py - 12, s, 12))

    def _series(self):
        fb = self.fb
        n = len(self.xs)
        if self.mode == 1:
            i = 0
            while i < n:
                px = self._mx(self.xs[i])
                py = self._my(self.ys[i])
                if self.px0 <= px < self.px0 + self.pw and self.py0 <= py < self.py0 + self.ph:
                    fb.pixel(px, py, 1)
                    fb.pixel(px + 1, py, 1)
                    fb.pixel(px, py + 1, 1)
                i += 1
        else:
            i = 1
            while i < n:
                x1 = self._mx(self.xs[i - 1])
                y1 = self._my(self.ys[i - 1])
                x2 = self._mx(self.xs[i])
                y2 = self._my(self.ys[i])
                fb.line(x1, y1, x2, y2, 1)
                i += 1

    def _overlay(self):
        # 文字必须在 lcd.blit 之后画：blit 会整屏覆盖（1bpp 帧缓冲没有文字层）
        for it in self._labels:
            x = it[0]
            y = it[1]
            s = it[2]
            size = it[3]
            if x < 0:
                x = 0
            if y < 0:
                y = 0
            if x + 6 * len(s) > self.w - 1:
                x = self.w - 1 - 6 * len(s)
            if y > self.h - size:
                y = self.h - size
            lcd.text(x, y, s, size)
        lcd.text(4, 1, self._title, 12)
        lcd.text(4, self.h - _BAR, "ON: back", 12)

    def show(self):
        self._bounds()
        self._frame()
        self._axes()
        self._series()
        lcd.blit(self.fb)
        self._overlay()
        lcd.wait_key()
        # after returning, the terminal is redrawn by the app on the next loop


# ---------- module level API ----------
def _g_():
    global _g
    if _g is None:
        _g = _Graph()
    return _g


def clf():
    _g_().clf()


def plot(a, b=None, c=None, d=None):
    _g_().plot(a, b, c, d)


def scatter(x, y):
    _g_().scatter(x, y)


def axis(xmin, xmax, ymin, ymax):
    _g_().axis(xmin, xmax, ymin, ymax)


def title(s):
    _g_().title(s)


def show():
    _g_().show()
