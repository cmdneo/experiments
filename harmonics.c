#include <stdio.h>
#include <stdlib.h>
#include <tgmath.h>
#include <inttypes.h>

/* Not using 2^15=32678 as double 1.0 is not exctly one nor is 2^15, so keeping the max/min value
 * oveflows/underflows int16_t and no sound is generated
 */
#define AMP_MAX 32670.0
#define RATE 44100.0
#define PI 3.1415926535897932384626433
#define DT (1.0 / RATE)
#define NHARMS 16
#define DAMP 0.004

enum audio_config {
	NBYTES = sizeof(int16_t) / sizeof(char),
};

/* stereo */
typedef union audio_T {
	char raw[NBYTES];
	int16_t amp;

} audio_T;

int main()
{
	audio_T aout = { 0 };
	double f = 100.;

	for (;;) {
		for (double t = 0.; t < 1.; t += DT) {
			double y = 0.;
			for (int i = 0; i < NHARMS; i++)
				y += sin((i + 1.) * 2. * PI * f * t) /
				     pow(2., (double)i);
			y /= 2.;
			y *= exp(-t * f * 2. * PI * DAMP);

			aout.amp = (int16_t)(AMP_MAX * y);
			fwrite(aout.raw, NBYTES, 1, stdout);
		}

		f += 25.;
	}

	return 0;
}
