#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <pigpio.h>

#include "max7221.h"
#include "font8x8/font8x8_basic.h" /* See font8x8/README for source info */

#define BITRATE 100000
#define SPI_CHANNEL 0
#define SPI_FLAGS 0x0
#define TX_BYTES 2
#define FRAME_DURATION_MS 100

void sleep_for(long ns) {
    struct timespec sleep_intv = {.tv_nsec = ns, .tv_sec = 0};
    nanosleep(&sleep_intv, 0);
}

char mirror_byte(char byte) {
    char ret = 0x0;
    char lmask = 0x80;
    char rmask = 0x01;

    for (int i = 7; i > 0; i -= 2) {
        ret |= ((byte & lmask) >> i) | ((byte & rmask) << i);
        lmask >>= 1;
        rmask <<= 1;
    }

    return ret;
}

void write_data(int handler, char reg, char data) {
    char buffer[] = {reg, data};
    assert(TX_BYTES == spiWrite(handler, buffer, TX_BYTES));

    sleep_for(MIN_CS_HIGH);
}

void write_frame(int handler, char frame[8]) {
    /* Mirror byte as dot matrix displays from right to left
     * can also be written in reverse order to the digit registers but no...
     */
    char data[8] = {0};
    for (int j = 0; j < 8; ++j) {
        data[j] = mirror_byte(frame[j]);
    }

    for (int i = 0; i < 8; ++i) {
        write_data(handler, DIG_REGS[i], data[i]);
    }
}

void scroll_text(int handler, char *txt, int n) {
    const int rows = n * 8;
    char (*fbuf)[8] = malloc(rows);

    for (int i = 0; i < n; i++)
        memcpy(fbuf[i], font8x8_basic[(int)txt[i]], 8);

    /* Horizontal scroll!!! */
    for (int i = 0; i < n * 8; i++) {
        memmove(fbuf[0], fbuf[0] + 1, rows);
        write_frame(handler, fbuf[0]);
        sleep_for(1000000 * FRAME_DURATION_MS);
    }
} 


int main(int argc, char* argv[argc + 1]) {
    int intensity = 1;

    if (argc < 2) {
        fprintf(
                stderr,
                "No input recieved\n"
                "Usage: %s \"Some text\" [intensity(%d-%d)]\n",
                argv[0], INTENSITY_MIN, INTENSITY_MAX
               );
        return EXIT_FAILURE;
    }
    else if (argc > 2) {
        intensity = strtol(argv[2], NULL, 10);

        if (intensity > INTENSITY_MAX || intensity < INTENSITY_MIN) {
            fprintf(stderr,
                    "Invalid value for intensity defauting to minimum value\n");
            intensity = INTENSITY_MIN;
        }
    }

    if (gpioInitialise() < 0) {
        fprintf(stderr, "GPIO initialization failed!\n");
        return EXIT_FAILURE;
    }

    int spihd = spiOpen(SPI_CHANNEL, BITRATE, SPI_FLAGS);
    if (spihd < 0) {
        fprintf(stderr, "Failed opening SPI channel\n");
        return EXIT_FAILURE;
    }

    /* Turn on if shutdown */
    write_data(spihd, SHUTDOWN_REG, SHUTDOWN_DISABLE);
    /* Set scan limit to display all lines  */
    write_data(spihd, SCAN_LIMIT_REG, SCAN_DIGS_0_7);
    /* Turn off Display test mode */
    write_data(spihd, DISPLAY_TEST_REG, DISPLAY_TEST_DISABLE);
    /* Set intensity */
    write_data(spihd, INTENSITY_REG, intensity);
    /* Clear the display */
    write_frame(spihd, font8x8_basic[0]);

    printf("Setup complete, writing:\n\'%s\' to led matrix.\n\n", argv[1]);

    // /* Text as first argument input */
    scroll_text(spihd, argv[1], strlen(argv[1]));

    /* Shutdown */
    write_data(spihd, SHUTDOWN_REG, SHUTDOWN_ENABLE);
    spiClose(spihd);
    gpioTerminate();

    printf("\nDone.\n");
    return EXIT_SUCCESS;
}
