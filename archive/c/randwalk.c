#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define STEPS 100000000

typedef struct point2d {
    int x;
    int y;
} point2d;

point2d add(point2d p1, point2d p2) {
    point2d ret = {.x = p1.x + p2.x, .y = p1.y + p2.y};
    return ret;
}

point2d sub(point2d p1, point2d p2) {
    point2d ret = {.x = p1.x - p2.x, .y = p1.y - p2.y};
    return ret;
}

point2d mul(point2d p1, point2d p2) {
    point2d ret = {.x = p1.x * p2.x, .y = p1.y * p2.y};
    return ret;
}

point2d smul(point2d p1, int scalar) {
    point2d ret = {.x = p1.x * scalar, .y = p1.y * scalar};
    return ret;
}

int main() {
    srand(time(0));
    point2d pt = { 0 };
    point2d directions[] = {{.x = 1}, {.x = -1}, {.y = 1}, {.y = -1}};
    point2d* path = calloc(STEPS + 1, sizeof *path);
    path[0] = pt;

    for(size_t i = 0; i < STEPS; i++) {
       int r = (int) (4.0 * (double) rand() / (double) RAND_MAX);
       pt = add(pt, directions[r]);
       // First element is starting point
       path[i + 1] = pt;
    }

    printf("Poisition {x = %d, y = %d}\n", pt.x, pt.y);

    return 0;
}
