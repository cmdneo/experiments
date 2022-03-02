#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
	struct timespec dr = { 0, 100000000 };
	int fd = open("/dev/console", O_WRONLY);
	if (fd < 0) {
		printf("Failed to open console\n");
		return 1;
	}

	for (int i = 0; i < 1000; i++) {
		if (i % 2 == 0)
			ioctl(fd, KDSETLED, 0x4U);
		else
			ioctl(fd, KDSETLED, 0x0U);
		nanosleep(&dr, NULL);
	}
}
