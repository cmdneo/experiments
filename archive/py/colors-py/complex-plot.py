import collections
import cmath
import colorsys
import math
import png
import random
import sys


GRID = ([-4, -4], [4, 4])
img = collections.deque()
step = 0.01


try:
    step = float(sys.argv[1])
except (ValueError, IndexError):
    pass


def frange(start, stop, step=0.1):
    t = start
    while 1:
        if t >= stop:
            return
        yield t
        t += step


def rgb_cresult(x, y):
    z = complex(x, y)
    res = z ** 3 - 1

    arg = cmath.phase(res) / math.tau
    arg = arg if arg > 0 else arg + 1
    abs_val = 2 * math.atan(abs(res)) / math.pi

    return tuple(map(
        lambda x: int(255 * x),
        colorsys.hsv_to_rgb(arg, 1, abs_val)
    ))

# WORKING CODE START
print("Starting...")

for y in frange(GRID[0][1], GRID[1][1], step):
    row = []
    for x in frange(GRID[0][0], GRID[1][0], step):
        row.extend(rgb_cresult(x, y))

    img.appendleft(row)
    print(f"\rProgress: {GRID[0][-1]} : {y:.4f} : {GRID[1][1]}", end="")

print("\nWriting image...")

with open(f"complex-{random.randint(0, 10 ** 10)}.png", "wb") as f:
    wrt = png.Writer(
        len(img[0]) // 3,
        len(img),
        greyscale=False
    )
    wrt.write(f, img)

print("\nDone.")
