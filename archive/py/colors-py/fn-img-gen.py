import png
import sys
import itertools
import random


GRID_SIZE = 256
SCALING = 1


def scale_img(scale, img):
    ret = []
    for row in img:
        tmp_row = []
        for i in range(0, len(row), 3):
            tmp_row.extend([row[i], row[i + 1], row[i + 2]] * scale)
        ret.extend([tmp_row] * scale)
    return ret


def main(fn_str_pair):
    img = []
    color_fn = lambda x, y: eval(fn_str_pair[0])

    print(f"Started..., {GRID_SIZE=:,}, {SCALING=}")

    for y in range(GRID_SIZE):
        row = []
        for x in range(GRID_SIZE):
            row.extend(color_fn(x, y))
        img.append(row)
        print(f"\rProgress: {y / GRID_SIZE:.2%}", end="")
    print(f"\rProgress: {1:.2%}", end="")

    if SCALING > 1:
        print("Scaling up image...")
        img = scale_img(SCALING, img)

    w, h = len(img[0]) // 3, len(img)
    print(f"\nWriting to image... {w=}, {h=}")

    wrt = png.Writer(w, h, greyscale=False)
    with open(f"fn-img-{fn_str_pair[1]}-{random.randint(0, 10 ** 10)}.png", "wb") as f:
        wrt.write(f, img)

    print("Done.")


if __name__ == "__main__":
    fn_str_pairs = [
        (f"(x {c[0]} y, x {c[1]} y, x {c[2]} y)", f"{c[0]}{c[1]}{c[2]}")
        for c in
        itertools.combinations_with_replacement(['^', '|', '&'], 3)
    ]
    try:
        for fn_str_pair in fn_str_pairs:
            main(fn_str_pair)
    except KeyboardInterrupt:
        print("Interrupted.")
        sys.exit(130)
