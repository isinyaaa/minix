#include "syslib.h"

/*===========================================================================*
 *                                sys_unlockpriority		     	     *
 *===========================================================================*/
PUBLIC int sys_unlockpriority(int proc)
{
  message m;

  m.m1_i1 = proc;
  return(_taskcall(SYSTASK, SYS_UNLOCKPRIORITY, &m));
}
