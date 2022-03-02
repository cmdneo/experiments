#include <stdio.h>
#include <stdlib.h>
#include <complex.h>

#define STEP 0.001
#define ITERS 200
#define RAMP_LEN 14
#define YRANGE (double[2]){-1.0, 1.0}
#define XRANGE (double[2]){-2.0, 2.0}

char const* CHAR_RAMP = " .,:;-~!*=$%#@";

int mand(double complex c){
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


int main() {
    // Because it prints from top so invert Y-axis
    for (double y = YRANGE[1]; y >= YRANGE[0]; y -= STEP) {
        for (double x = XRANGE[0]; x <= XRANGE[1]; x += STEP) {
            int res = (int) ((RAMP_LEN - 1) * (double) mand(CMPLX(x, y)) / ITERS);
            printf("%c%c", CHAR_RAMP[res], CHAR_RAMP[res]);
        }
        printf("\n");
    }

    return EXIT_SUCCESS;
}
