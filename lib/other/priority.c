/*
priority.c
*/

#include <errno.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <lib.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>


int lockpriority(int who, int prio)
{
	message m;

	m.m1_i1 = who;
	m.m1_i2 = prio;

	return _syscall(MM, LOCKPRIORITY, &m);
}


int unlockpriority(int who)
{
	message m;

	m.m1_i1 = who;

	return _syscall(MM, UNLOCKPRIORITY, &m);
}

