#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>

#define NELEMS 10000000 // 10 Million
#define NITERS 100

int count_larger(int *arr, int items, int big_than)
{
	int count = 0;
	for (int i = 0; i < items; i++) {
		if (arr[i] > big_than)
			count++;
	}

	return count;
}

double bench_branch(unsigned int seed)
{
	srand(seed);
	struct timespec st;
	struct timespec et;
	int *arr = malloc((sizeof *arr) * NELEMS);
	int find_me = rand();
	double find_me_dev = 2. * (100. * find_me / RAND_MAX - 50.);

	if (arr == NULL) {
		fprintf(stderr, "Memory allocation error!!1 FATAL.");
		exit(1);
	}

	for (int i = 0; i < NELEMS; i++)
		arr[i] = i;
	clock_gettime(CLOCK_MONOTONIC, &st);
	count_larger(arr, NELEMS, find_me);
	clock_gettime(CLOCK_MONOTONIC, &et);
	double time_seq = (double)(et.tv_sec - st.tv_sec) + et.tv_nsec / 1000000000. -
			  st.tv_nsec / 1000000000.;

	for (int i = 0; i < NELEMS; i++)
		arr[i] = rand();
	clock_gettime(CLOCK_MONOTONIC, &st);
	count_larger(arr, NELEMS, find_me);
	clock_gettime(CLOCK_MONOTONIC, &et);
	double time_rnd = (double)(et.tv_sec - st.tv_sec) + et.tv_nsec / 1000000000. -
			  st.tv_nsec / 1000000000.;
	double diff = time_rnd - time_seq;
	double percent = 100. * diff / time_rnd;

	printf("\r");
	printf("Seq : %10Fs    Rnd : %10Fs\n", time_seq, time_rnd);
	printf("Diff: %10Fs    Diff:   %0+6.2F%%    Rnd_dev: %0+6.2F%%\n", diff, percent,
	       find_me_dev);
	printf("------------------------------------------------------------------\n");

	free(arr);
	return percent;
}

int main(int argc, char const *argv[])
{
	setlocale(LC_NUMERIC, "");
	printf("Starting branch predition test...\nwith %d elements and %d iterations:\n\n",
	       NELEMS, NITERS);
	double avg = 0;

	for (int i = 0; i < NITERS; i++) {
		unsigned int seed = time(NULL) + i;
		printf("=> %d/%d.....", i + 1, NITERS);
		fflush(stdout);
		avg += bench_branch(seed);
	}

	avg = avg / NITERS;
/* Locale numeric comma flag is only supported on posix systems so... */
#if __unix__
	printf("INFO: %'d elements, %'d iterations\n", NELEMS, NITERS);
#else
	printf("INFO: %d elements, %d iterations\n", NELEMS, NITERS);
#endif
	printf("Average %.2F%% speedup\n", avg);

	return 0;
}
