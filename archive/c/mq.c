#include <stdio.h>
#include <stdlib.h>


void test() {
    static unsigned sa = 10;
    sa += 5;
    printf("%u\n", sa);
}

int main() {
    for(size_t i = 0; i < 10; i++)
        test();
    return EXIT_SUCCESS;
}
