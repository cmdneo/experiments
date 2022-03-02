import sys


step = 0.01
GRID = ([-2, -1], [2, 1])
char_ramp = " .,:;-~!*=$%#@"
ramp_range = len(char_ramp) - 1

try:
    ITERS = int(sys.argv[1])
except (IndexError, ValueError):
    ITERS = 200


def mandelbrot(comp, iterations):
    z = 0
    for i in range(iterations):
        z = z ** 2 + comp
        if z.real ** 2 + z.imag ** 2 >= 4:
            return i
    
    return iterations


def color(text, val):
    factor = val / ITERS

    r = int(255 * factor)
    g = int(255 * (1 - factor))
    b = 0
    return f"\x1b[38;2;{r};{g};{b}m{text}\x1b[0m"


y = GRID[0][1]
x = GRID[0][0]
while y <= GRID[1][1]:
    line = ""
    while x <= GRID[1][0]:
        res = mandelbrot(complex(x, y), ITERS)
        line += char_ramp[int((ramp_range * res) / ITERS)] + " "
        x += step
    print(line.rstrip())
    x = GRID[0][0]
    y += step

