#include "syslib.h"

/*===========================================================================*
 *                                sys_lockpriority		     	     *
 *===========================================================================*/
PUBLIC int sys_lockpriority(int proc, int prio)
{
  message m;

  m.m1_i1 = proc;
  m.m1_i2 = prio;
  return(_taskcall(SYSTASK, SYS_LOCKPRIORITY, &m));
}
