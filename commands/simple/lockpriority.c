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
	int priority;

	pid = atoi(argv[1]);
	if (pid <= 0)
		usage();

	priority = atoi(argv[2]);

	if (lockpriority(pid, priority)) {
		perror("lockpriority");
		return 1;
	}
	return 0;
}

void
usage(void)
{
	(void)fprintf(stderr, "usage: lockpriority PID PRIORITY\n");
	exit(1);
}
