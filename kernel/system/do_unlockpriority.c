/* The kernel call implemented in this file:
 *   m_type:	SYS_UNLOCKPRIORITY
 *
 * The parameters for this kernel call are:
 *    m1_i1:	PR_ENDPT   	process number to unlock
 */

#include "../system.h"
#include <minix/type.h>
#include <sys/resource.h>

#if USE_LOCKPRIORITY

/*===========================================================================*
 *				  do_unlockpriority			     *
 *===========================================================================*/
PUBLIC int do_unlockpriority(message *m_ptr)
{
  int proc_nr;
  register struct proc *rp;

  /* Extract the message parameters and do sanity checking. */
  if(!isokendpt(m_ptr->PR_ENDPT, &proc_nr)) return EINVAL;
  if (iskerneln(proc_nr)) return(EPERM);
  rp = proc_addr(proc_nr);

   rp->p_priority_locked = 0;

   return(OK);
}

#endif /* USE_LOCKPRIORITY */

