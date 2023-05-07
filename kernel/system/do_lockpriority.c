/* The kernel call implemented in this file:
 *   m_type:	SYS_LOCKPRIORITY
 *
 * The parameters for this kernel call are:
 *    m1_i1:	PR_ENDPT   	process number to change priority
 *    m1_i2:	PR_PRIORITY	the priority to lock the process to (7-14)
 */

#include "../system.h"
#include <minix/type.h>
#include <sys/resource.h>

#if USE_LOCKPRIORITY

/*===========================================================================*
 *				  do_lockpriority			     *
 *===========================================================================*/
PUBLIC int do_lockpriority(message *m_ptr)
{
  int proc_nr, pri;
  register struct proc *rp;

  /* Extract the message parameters and do sanity checking. */
  if(!isokendpt(m_ptr->PR_ENDPT, &proc_nr)) return EINVAL;
  if (iskerneln(proc_nr)) return(EPERM);
  pri = m_ptr->PR_PRIORITY;
  rp = proc_addr(proc_nr);

  /* if (pri < USER_Q || pri > MIN_USER_Q) */
  /*     return(EINVAL); */

  /* Make sure the process is not running while changing its priority. 
   * Put the process back in its new queue if it is runnable.
   */
  lock_dequeue(rp);
  rp->p_max_priority = rp->p_priority = pri;
  rp->p_priority_locked = 1;
  if (! rp->p_rts_flags) lock_enqueue(rp);

  return(OK);
}

#endif /* USE_LOCKPRIORITY */

