import ctypes
import time
import sys
import png
from random import randint


DLIB = "./mand.so"
XRANGE = [-2.0, 0.8]
YRANGE = [-1.2, 1.2]
iters = 100
step = 0.02

try:
    libm = ctypes.CDLL(DLIB)
except FileNotFoundError:
    print("Failed to load lib")
    exit(1)

try:
    step, iters = float(sys.argv[1]), int(sys.argv[2])
except (ValueError, IndexError):
    print(f"CLI args error!\nFormat: {sys.argv[0]} STEP ITERS")
    sys.exit(1)


print("Starting...")
img_size = [
    int(abs(XRANGE[0] - XRANGE[1]) / step),
    int(abs(YRANGE[0] - YRANGE[1]) / step)
]
gen_mand = libm.gen_mandelbrot
gen_mand.argtypes = [
    ctypes.POINTER(ctypes.c_double * 2),
    ctypes.POINTER(ctypes.c_double * 2),
    ctypes.POINTER(ctypes.c_int * 2),
    ctypes.c_double,
    ctypes.c_int
]
gen_mand.restype = ctypes.POINTER(ctypes.c_int * (img_size[0] * img_size[1]))

start = time.time()
img_ptr = gen_mand(
        ctypes.byref((ctypes.c_double * 2)(*XRANGE)),
        ctypes.byref((ctypes.c_double * 2)(*YRANGE)),
        ctypes.byref((ctypes.c_int * 2)(*img_size)),
        ctypes.c_double(step),
        ctypes.c_int(iters)
)
total_time = time.time() - start
print(f"Time taken by lib: {total_time:.3f}s")

if not img_ptr:
    print("NULL pointer returned!!!")
    sys.exit(1)

tmp_content = img_ptr.contents
img = [[tmp_content[y * img_size[0] + x] for x in range(img_size[0])]
        for y in range(img_size[1])]

wrt = png.Writer(
        img_size[0],
        img_size[1],
        greyscale=True
        )
with open(f"ID{randint(10, 10 ** 6)}-{iters}-mand.png", "wb") as f:
    wrt.write(f, img)

print("\nDone.")
