#include <sys/types.h>
#include <sys/resource.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void usage(void);

int
main(int argc, char *argv[])
{
	pid_t pid;

	pid = atoi(argv[1]);
	if (pid <= 0)
		usage();

	if (unlockpriority(pid)) {
		perror("unlockpriority");
		return 1;
	}
	return 0;
}

void
usage(void)
{
	(void)fprintf(stderr, "usage: unlockpriority PID\n");
	exit(1);
}
