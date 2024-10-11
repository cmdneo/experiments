/*
Pipe output into aplay with options "-fcd -c1", like so:

gcc -lm harmonics.c -o harmonics
./harmonics | aplay -fcd -c1
*/

#include <stdio.h>
#include <stdlib.h>
#include <tgmath.h>
#include <inttypes.h>

/* Keeping the max/min value for AMP
 * oveflows/underflows int16_t and no sound is generated.
 */
#define AMP_MAX 32670.0
#define RATE 44100.0
#define PI 3.14159265
#define DT (1.0 / RATE)
#define NHARMS 8
#define DAMP 0.001
#define TONE_TIME 1.5

typedef union audio_T {
	char raw[sizeof(int16_t) / sizeof(char)];
	int16_t amp;
} audio_T;

int main(void)
{
	audio_T aout = {0};
	double f = 100.;

	for (;;) {
		for (double t = 0.; t < TONE_TIME; t += DT) {
			double y = 0.;
			for (int i = 0; i < NHARMS; i++)
				y += sin((i + 1.) * 2. * PI * f * t) / pow(2., (double)i);
			y /= 2.;
			y *= exp(-t * f * 2. * PI * DAMP);

			aout.amp = (int16_t)(AMP_MAX * y);
			fwrite(aout.raw, sizeof(audio_T), 1, stdout);
		}

		f += 50.;
	}

	return 0;
}
