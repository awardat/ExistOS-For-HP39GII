# graph3d.py v2026-10-08 - minimal 3D plotting module for the ExistOS Python app
#
# ASCII-only source (MicroPython reads UTF-8; the LCD font is GBK-indexed).
# 1bpp screen: wireframe rendering (lines), no shading.
#
# API:
#   clf()                                 clear the current figure
#   surface(f, x0, x1, y0, y1, n=14)      z = f(x, y) over a grid (wireframe mesh)
#   plot(xs, ys, zs)                      3D polyline from three sequences
#   view(az, el)                          camera angles in degrees (default 45, 30)
#   title(s)                              figure title (ASCII)
#   show()                                render full screen; L/R rotate, U/D tilt,
#                                         ON/F5 back to the terminal
#
# Example:
#   import graph3d, math
#   graph3d.surface(lambda x, y: math.sin(x) * math.cos(y), -3, 3, -3, 3)
#   graph3d.show()

import framebuf
import lcd
import math

_TOP = 14            # leave the top icon row to the system
_BAR = 12            # bottom hint line

_g = None


class _Graph3d(object):
    def __init__(self):
        self.w = lcd.width()
        self.h = lcd.height()
        self.px0 = 0
        self.py0 = _TOP
        self.pw = self.w
        self.ph = self.h - _TOP - _BAR
        self.segs = []          # flat list of (x1,y1,z1,x2,y2,z2)
        self._title = ""
        self.az = 45.0          # azimuth (rotation, degrees)
        self.el = 30.0          # elevation (tilt, degrees)
        self.cx = self.cy = self.cz = 0.0
        self.inv = 1.0

    # ---------- figure state ----------
    def clf(self):
        self.segs = []
        self._title = ""
        self.az = 45.0
        self.el = 30.0

    def title(self, s):
        self._title = s

    def view(self, az, el):
        self.az = float(az) % 360.0
        el = float(el)
        if el > 89.0:
            el = 89.0
        if el < -89.0:
            el = -89.0
        self.el = el

    # ---------- data ----------
    def surface(self, f, x0, x1, y0, y1, n=14):
        x0 = float(x0)
        x1 = float(x1)
        y0 = float(y0)
        y1 = float(y1)
        n = int(n)
        if n < 4:
            n = 4
        if n > 30:
            n = 30
        if x1 <= x0 or y1 <= y0:
            raise ValueError("bad range")
        zs = []
        i = 0
        while i < n:
            x = x0 + (x1 - x0) * i / (n - 1)
            row = []
            j = 0
            while j < n:
                y = y0 + (y1 - y0) * j / (n - 1)
                row.append(float(f(x, y)))
                j += 1
            zs.append(row)
            i += 1
        # mesh: lines along x and along y
        i = 0
        while i < n:
            x = x0 + (x1 - x0) * i / (n - 1)
            j = 0
            while j < n - 1:
                ya = y0 + (y1 - y0) * j / (n - 1)
                yb = y0 + (y1 - y0) * (j + 1) / (n - 1)
                self.segs.append((x, ya, zs[i][j], x, yb, zs[i][j + 1]))
                self.segs.append((ya, x, zs[j][i], yb, x, zs[j + 1][i]))
                j += 1
            i += 1

    def plot(self, xs, ys, zs):
        if xs is None or ys is None or zs is None:
            raise ValueError("x/y/z required")
        if len(xs) != len(ys) or len(xs) != len(zs) or len(xs) < 2:
            raise ValueError("x/y/z must be sequences of equal length (>= 2)")
        i = 1
        while i < len(xs):
            self.segs.append((float(xs[i - 1]), float(ys[i - 1]), float(zs[i - 1]),
                              float(xs[i]), float(ys[i]), float(zs[i])))
            i += 1

    # ---------- projection ----------
    def _bounds(self):
        if not self.segs:
            return
        xmin = xmax = self.segs[0][0]
        ymin = ymax = self.segs[0][1]
        zmin = zmax = self.segs[0][2]
        for s in self.segs:
            for k in range(0, 6, 3):
                x = s[k]
                y = s[k + 1]
                z = s[k + 2]
                if x < xmin:
                    xmin = x
                if x > xmax:
                    xmax = x
                if y < ymin:
                    ymin = y
                if y > ymax:
                    ymax = y
                if z < zmin:
                    zmin = z
                if z > zmax:
                    zmax = z
        self.cx = (xmin + xmax) * 0.5
        self.cy = (ymin + ymax) * 0.5
        self.cz = (zmin + zmax) * 0.5
        ext = xmax - xmin
        if ymax - ymin > ext:
            ext = ymax - ymin
        if zmax - zmin > ext:
            ext = zmax - zmin
        if ext < 1e-12:
            ext = 1e-12
        self.inv = 1.0 / ext
        self.axs = ((xmin, xmax), (ymin, ymax), (zmin, zmax))

    def _proj(self, x, y, z):
        # normalize (common scale, no distortion), rotate around z, tilt, orthographic
        nx = (x - self.cx) * self.inv
        ny = (y - self.cy) * self.inv
        nz = (z - self.cz) * self.inv
        a = self.az * 0.017453292519943295
        e = self.el * 0.017453292519943295
        ca = math.cos(a)
        sa = math.sin(a)
        ux = ca * nx - sa * ny
        uy = sa * nx + ca * ny
        v = math.cos(e) * nz - math.sin(e) * uy
        return ux, v

    def _to_px(self, u, v, sc, mx, my):
        return int(mx + u * sc + 0.5), int(my - v * sc + 0.5)

    # ---------- drawing ----------
    def _draw(self, fb):
        sc = self.ph * 0.92
        mx = self.px0 + self.pw * 0.5
        my = self.py0 + self.ph * 0.5
        # axes from the box min corner (before the data, so data draws over them)
        if getattr(self, "axs", None) is not None:
            (x0, x1), (y0, y1), (z0, z1) = self.axs
            o = self._proj(x0, y0, z0)
            ox, oy = self._to_px(o[0], o[1], sc, mx, my)
            for pt, lab in ((self._proj(x1, y0, z0), "x"),
                            (self._proj(x0, y1, z0), "y"),
                            (self._proj(x0, y0, z1), "z")):
                px, py = self._to_px(pt[0], pt[1], sc, mx, my)
                fb.line(ox, oy, px, py, 1)
                self._axis_labels.append((px, py, lab))
        for s in self.segs:
            u1, v1 = self._proj(s[0], s[1], s[2])
            u2, v2 = self._proj(s[3], s[4], s[5])
            x1, y1 = self._to_px(u1, v1, sc, mx, my)
            x2, y2 = self._to_px(u2, v2, sc, mx, my)
            fb.line(x1, y1, x2, y2, 1)

    def _overlay(self):
        # text must be drawn after lcd.blit (the 1bpp frame buffer has no text layer)
        for x, y, lab in self._axis_labels:
            if 0 <= x < self.w - 6 and 0 <= y < self.h - 12:
                lcd.text(x + 2, y, lab, 12)
        lcd.text(4, 1, self._title, 12)
        s = "az%d el%d" % (int(self.az), int(self.el))
        lcd.text(self.w - 2 - 8 * len(s), 1, s, 12)
        lcd.text(4, self.h - _BAR, "ON:back L/R:rot U/D:tilt", 12)

    def show(self):
        self._bounds()
        fb = framebuf.FrameBuffer(bytearray((self.w * self.h + 7) // 8),
                                  self.w, self.h, framebuf.MONO_HLSB)
        while True:
            self._axis_labels = []
            fb.fill(0)
            fb.rect(0, 0, self.w, self.h, 1)
            fb.line(0, _TOP - 1, self.w - 1, _TOP - 1, 1)
            self._draw(fb)
            lcd.blit(fb)
            self._overlay()
            k = lcd.wait_nav()
            if k == 0:
                return
            if k == 1:
                self.az -= 15.0
            elif k == 2:
                self.az += 15.0
            elif k == 3:
                self.el += 10.0
            elif k == 4:
                self.el -= 10.0
            if self.el > 89.0:
                self.el = 89.0
            if self.el < -89.0:
                self.el = -89.0


# ---------- module level API ----------
def _g_():
    global _g
    if _g is None:
        _g = _Graph3d()
    return _g


def clf():
    _g_().clf()


def surface(f, x0, x1, y0, y1, n=14):
    _g_().surface(f, x0, x1, y0, y1, n)


def plot(xs, ys, zs):
    _g_().plot(xs, ys, zs)


def view(az, el):
    _g_().view(az, el)


def title(s):
    _g_().title(s)


def show():
    _g_().show()
