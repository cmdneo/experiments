#ifndef MAND_H
#define MAND_H

int mand_iters(double complex c, int const ITERS);

extern int* gen_mandelbrot(
        double const XRANGE[static 2],
        double const YRANGE[static 2],
        int const IMG_SIZE[static 2],
        double const STEP,
        int const ITERS);
#endif
