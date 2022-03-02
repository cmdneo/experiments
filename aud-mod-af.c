#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>

#define AMP_MAX ((double)INT16_MAX)
#define RATE 44100.0
#define PI 3.1415926535897932384626433

#define DT (1.0 / RATE)

typedef union audio_T {
	int16_t amp;
	uint8_t raw[sizeof(int16_t)];
} audio_T;

int main()
{
	audio_T aud;
	double t = 0.;

	while (1) {
		fread(&aud, sizeof(aud), 1, stdin);

		//aud.amp = (int16_t)(aud.amp *
		//		    sin(2 * PI * 5000. * t)); /* AMP mod */
		 aud.amp = (int16_t)(AMP_MAX *
				    sin(2 * PI *
		 			(5000. + 2000. * (aud.amp / AMP_MAX)) *
		 			t)); /* FREQ mod */

		fwrite(&aud, sizeof(aud), 1, stdout);

		t += DT;
	}
}
