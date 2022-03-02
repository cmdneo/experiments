import png
import random
import sys
import threading
import time

# TODO : int(sys.argv[0]), int(sys.argv[1])
STEPS = 100_000
GRID_SIZE = 100
SCALING = 3
DIRECTIONS = [(0, 1), (1, 0), (-1, 0), (0, -1)]
PRG_var = 0     # global

get_uv = lambda x, y: (2 * (x + GRID_SIZE // 2), 2 * (y + GRID_SIZE // 2))


def scale_img(scale, img):
    ret = []
    for row in img:
        tmp_row = []
        for c in row:
            tmp_row.extend([c] * scale)
        ret.extend([tmp_row] * scale)
    return ret


def show_progress(delay, dfac):
    while 0 <= PRG_var:
        print(f"\rProgress: {PRG_var / dfac:.2%}", end='')
        time.sleep(delay)
    if PRG_var == -42:
        print(f"\rProgress: {1:.2%}", end='')
    print()


def main():
    global PRG_var
    coords = [0, 0]
    # 2 times for gaps
    tmp_row = [0 for x in range(2 * GRID_SIZE)]
    img_size = 2 * GRID_SIZE
    img = [tmp_row.copy() for y in range(2 * GRID_SIZE)]

    print(f"Started..., {STEPS=:,}, {GRID_SIZE=:,}, {SCALING=}")
    # For tracking progress, no GIL when read only op
    t = threading.Thread(target=show_progress, args=(0.1, STEPS))
    t.start()

    for i in range(STEPS):
        wd = random.choice(DIRECTIONS)
        PRG_var = i

        u1, v1 = get_uv(*coords)
        uw, vw = u1 + wd[0], v1 + wd[1]
        img[u1 % img_size][v1 % img_size] = 255
        img[uw % img_size][vw % img_size] = 255
        coords[0] += wd[0]
        coords[1] += wd[1]

    # Tell progress tracker op->success
    PRG_var = -42
    t.join()

    print("Paths and image done.")
    print("Scaling up image...")
    img = scale_img(SCALING, img)
    print("\nWriting to image...")


    wrt = png.Writer(len(img[0]), len(img), greyscale=True)
    with open(f"rand-walk-{STEPS}-{GRID_SIZE}-{random.randint(0, 10 ** 10)}.png", "wb") as f:
        wrt.write(f, img)

    print("Done.")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        PRG_var = -1
        print("Interrupted.")
