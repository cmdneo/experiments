import png
import math
import sys

char_ramp = ' .:;~!+^*=%$#@'
ramp_range = len(char_ramp) - 1
try:
    # Shrink dimensions by
    shrink = int(sys.argv[1])
except (IndexError, ValueError):
    shrink = 4

f = open("img.png", "rb")
reader = png.Reader(file=f)
width, height, rows, info = reader.read()
rows = list(rows)
max_val = 2 ** info["bitdepth"] - 1
chunk_size = 3 - 2 * info["greyscale"] + info["alpha"] * 1


for b_row in rows[::shrink]:
    row = list(b_row)
    for i in range(0, len(row) - chunk_size, chunk_size * shrink):
        px = 0.0
        if info["greyscale"]:
            px = row[i] / 255
        else:
            px = (0.2989 * row[i] + 0.5870 * row[i + 1] + 0.1140 * row[i + 3]) / 255
        print(char_ramp[int(ramp_range * px)]  ,end=" ")
    print()

f.close()
