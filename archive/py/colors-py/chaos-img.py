import math
import sys
import png
import array
import random


GRID = ([-1000, -1000], [1000, 1000])
img = [array.array("B", [0 for x in range(abs(GRID[0][0]) + abs(GRID[1][0]))])
          for y in range(abs(GRID[0][1]) + abs(GRID[1][1]))]

def fn(t, x, y):
    x = x * t - y ** 2 + t
    y = x ** 2 * t - y - t
    return (x, y)



# WORKING CODE START
print("Starting...")
t = 0

while t < 10:
    x, y = fn(t, t, t)
    xt = x - GRID[0][0]
    yt = y - GRID[0][1]
    if xt < GRID[1][0] and yt < GRID[1][1]:
        img[int(yt)][int(xt)] = int(255 * t / (t + 3))
    t += 0.01
    print(f"\rT: {t}", end="")

print("\nWriting image...")

wrt = png.Writer(
        len(img[0]),
        len(img),
        greyscale=True
        )
with open(f"{random.randint(10, 10 ** 10)}-eq-ch.png", "wb") as f:
    wrt.write(f, img)

print("\nDone.")
