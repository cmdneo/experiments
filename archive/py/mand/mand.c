#include <stdio.h>
#include <stdlib.h>
#include <complex.h>

#include "mand.h"

#define LIKELY(x) __builtin_expect(x, 1)
#define UNLIKELY(x) __builtin_expect(x, 0)

int mand_iters(double complex c, int const ITERS){
    int iters = 0;
    double complex z = c;

    for (int i = 0; i < ITERS; i++) {
        z = z * z + c;
        // ~2.6 times faster than cabs(z) >= 2
        if (creal(z) * creal(z) + cimag(z) * cimag(z) >= 4)
            return i;
    }

    return ITERS;
}

int* gen_mandelbrot(
        double const XRANGE[static 2],
        double const YRANGE[static 2],
        int const IMG_SIZE[static 2],
        double const STEP,
        int const ITERS
) {
    size_t img_index = 0;
    size_t img_pixels = IMG_SIZE[0] * IMG_SIZE[1];
    int* img = calloc(img_pixels, sizeof(int));
    if (!img) {
        printf("Allocation fail!\n");
        return 0;
    }

    // ~1.25 times faster but with large/small values y'll get FP problems
    //for (double y = YRANGE[1]; y > YRANGE[0]; y -= STEP) {
    //    for (double x = XRANGE[0]; x < XRANGE[1]; x += STEP) {
    for (int k = 0; k < IMG_SIZE[1]; k++) {
        //printf("\rProgress: %d out of %d", k, IMG_SIZE[1]);
        double y = YRANGE[1] - STEP * k;
        for (int h = 0; h < IMG_SIZE[0]; h++) {
            double x = XRANGE[0] + STEP * h;
            int res = (int) ((255.0 * mand_iters(CMPLX(x, y), ITERS)) / ITERS);
            img[img_index] = res;
            img_index++;
        }
    }

    if (img_index != img_pixels) {
        printf("TOTAL: %zu, NEEDED: %zu\n", img_index, img_pixels);
        printf(
"Floating point skipped or did extra steps due to its nature \
as it is used in loop variables\n\
If you are seeing this then \x1b[1;31mRUN FOR YOUR LIFE!\x1b[0m\n"
);
        return 0;
    }

    return img;
}
