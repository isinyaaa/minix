/* This file deals with creating processes (via FORK) and EXECing new ones.
 * When a process forks, a new slot in the 'mproc' table is allocated for it,
 * and a copy of the parent's core image is made for the child.  Then the kernel
 * and file system are informed. When a process EXECs a new program, the
 * following takes place:
 *    - see if the permissions allow the file to be executed
 *    - read the header and extract the sizes
 *    - fetch the initial args and environment from the user space
 *    - allocate the memory for the new process
 *    - copy the initial stack from PM to the process
 *    - read in the text and data segments and copy to the process
 *    - take care of setuid and setgid bits
 *    - fix up 'mproc' table
 *    - tell kernel about EXEC
 *    - save offset to initial argc (for ps)
 *
 * The entry point into this file are:
 *   do_exec:	 perform the EXEC system call
 *   do_fork:	 perform the FORK system call
 */

#include "pm.h"
#include <sys/wait.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include <sys/resource.h>
#include <signal.h>
#include <stdlib.h>
#include "mproc.h"
#include "../../kernel/config.h"

extern int next_child;

FORWARD _PROTOTYPE (void update_text_share, (struct mproc *rmp, phys_clicks new_base) );

/*===========================================================================*
 *				compact_mem				     *
 *===========================================================================*/
PUBLIC int compact_mem()
{
/* Compact memory. Start from the topmost address on mproc and go down. For
 * every slot in use we alloc_endmem. If the process has a T segment, we
 * move it too
 */
	register struct mproc *rmp;
	int s, r, shared;
	phys_clicks prog_clicks, new_base;
	phys_bytes prog_bytes, old_abs, new_abs;	/* Intel only */
	int mpi;

	for (mpi = next_child; mpi > 0; mpi -= 1) {
		rmp = &mproc[mpi];
		/* assume the process that is compacting will end soon */
		if (rmp == mp)
			continue;
		if(!(rmp->mp_flags & IN_USE))
			continue;
		if (rmp->mp_nice <= mproc[0].mp_nice) {
			/* printf("PM: compact_mem: skipping unnice %d\n", mpi); */
			continue;
		}
		if (rmp->mp_flags & ZOMBIE) {
			/* printf("PM: compact_mem: skipping zombie %d\n", mpi); */
			continue;
		}
		prog_clicks = (phys_clicks) rmp->mp_seg[S].mem_len;
		prog_clicks += (rmp->mp_seg[S].mem_vir - rmp->mp_seg[D].mem_vir);
		if (rmp->mp_flags & SEPARATE) {
			if (find_share(rmp, rmp->mp_ino, rmp->mp_dev, rmp->mp_ctime) == NULL) {
				shared = 0;
				old_abs = (phys_bytes) rmp->mp_seg[T].mem_phys << CLICK_SHIFT;
				prog_clicks += (phys_clicks) rmp->mp_seg[T].mem_len;
			} else {
				shared = 1;
				old_abs = (phys_bytes) rmp->mp_seg[D].mem_phys << CLICK_SHIFT;
			}
		} else {
			shared = -1;
			old_abs = (phys_bytes) rmp->mp_seg[D].mem_phys << CLICK_SHIFT;
		}

		new_base = alloc_endmem(prog_clicks);
		if (new_base == NO_MEM) return ENOMEM;
		/* we can't move the process */
		if (new_base == -1) continue;

		new_abs = (phys_bytes) new_base << CLICK_SHIFT;
		prog_bytes = (phys_bytes) prog_clicks << CLICK_SHIFT;

		if (old_abs + prog_bytes == new_abs || new_abs + prog_bytes == old_abs) {
#if VERBOSE_VM
			printf("PM: compact_mem: %d already compacted\n", mpi);
#endif
			free_mem(new_base, prog_clicks);
			continue;
		}

		s = sys_abscopy(old_abs, new_abs, prog_bytes);
		if (s < 0) panic(__FILE__,"do_compact_mem: can't copy", s);

		free_mem(rmp->mp_seg[D].mem_phys,
			 rmp->mp_seg[S].mem_vir + rmp->mp_seg[S].mem_len - rmp->mp_seg[D].mem_vir);

		switch (shared) {
			case -1: /* combined */
				rmp->mp_seg[T].mem_phys = new_base;
				rmp->mp_seg[D].mem_phys = new_base;
#if VERBOSE_VM
				printf("compact_mem: %s (%d) combined of %dK -> %dK (%dK)\n",
					rmp->mp_name, mpi,
					old_abs/1024,
					(new_base << CLICK_SHIFT)/1024,
					(prog_clicks << CLICK_SHIFT)/1024);
#endif
				break;
			case 0: /* owner */
				update_text_share(rmp, new_base);
				free_mem(rmp->mp_seg[T].mem_phys, rmp->mp_seg[T].mem_len);
				rmp->mp_seg[T].mem_phys = new_base;
				rmp->mp_seg[D].mem_phys = new_base + rmp->mp_seg[T].mem_len;
#if VERBOSE_VM
				printf("compact_mem: %s (%d) owner of %dK -> %dK (%dK)\n",
					rmp->mp_name, mpi,
					old_abs/1024,
					(new_base << CLICK_SHIFT)/1024,
					(prog_clicks << CLICK_SHIFT)/1024);
#endif
				break;
			case 1: /* shared */
				rmp->mp_seg[D].mem_phys = new_base;
#if VERBOSE_VM
				printf("compact_mem: %s (%d) shared of %dK -> %dK (%dK)\n",
					rmp->mp_name, mpi,
					old_abs/1024,
					(new_base << CLICK_SHIFT)/1024,
					(prog_clicks << CLICK_SHIFT)/1024);
#endif
				break;
			default:
				break;
		}

		rmp->mp_seg[S].mem_phys = rmp->mp_seg[D].mem_phys +
			(rmp->mp_seg[S].mem_vir - rmp->mp_seg[D].mem_vir);

		if((r=sys_newmap(rmp->mp_endpoint, rmp->mp_seg)) != OK) {
			panic(__FILE__,"do_compact_mem: can't sys_newmap", r);
		}
	}

	return(OK);
}

PRIVATE void update_text_share(rmp, new_base)
struct mproc *rmp;
phys_clicks new_base;
{
	register struct mproc *rms;
	int mpi, r;

	for (mpi = 0; mpi < NR_PROCS; mpi++) {
		rms = &mproc[mpi];
		if (!(rms->mp_flags & IN_USE))
			continue;
		if (rms == rmp)
			continue;
		if (rms->mp_seg[T].mem_phys != rmp->mp_seg[T].mem_phys)
			continue;

		rms->mp_seg[T].mem_phys = new_base;
		if((r=sys_newmap(rms->mp_endpoint, rms->mp_seg)) != OK) {
			printf("PM: compact_mem: mp: %d flags: 0x%04x\n", mpi, rms->mp_flags);
			panic(__FILE__,"do_compact_mem: can't sys_newmap", r);
		}
	}
}
