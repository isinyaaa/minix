/* Author: Isabella Basso <isabellabdoamaral@usp.br>  08 june 2023 */

#define _MINIX 1
#define _POSIX_SOURCE 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <sys/types.h>

#include <minix/ipc.h>
#include <minix/config.h>
#include <minix/type.h>
#include <minix/const.h>

#include "../../servers/pm/mproc.h"
#include "../../kernel/const.h"
#include "../../kernel/proc.h"

#define PROCS (NR_PROCS+NR_TASKS)

static struct pb {
	struct proc *p;
	struct mproc *mp;
	long base;
	long len;
} membase_procs[PROCS];

int cmp_base(const void *v1, const void *v2)
{
	struct pb *p1 = (struct pb *) v1, *p2 = (struct pb *) v2;
	if(p1->base > p2->base)
		return 1;
	if(p1->base < p2->base)
		return -1;
	/* this should not happen */
	return 0;
}

int cmp_h_base(const void *v1, const void *v2)
{
	struct hole *h1 = (struct hole *) v1, *h2 = (struct hole *) v2;
	if(h1->h_base > h2->h_base)
		return 1;
	if(h1->h_base < h2->h_base)
		return -1;
	/* this should not happen */
	return 0;
}

void print_mem(struct proc *proc, struct mproc *mproc, struct pm_mem_info *pmi)
{
        int pi, hi, nprocs, nholes, last_hole;
	struct proc *p;
	struct mproc *mp;
	struct hole *h;
	long base, len;

	for(pi = nprocs = 0; pi < PROCS; pi++) {
		p = &proc[pi];
		if(p->p_rts_flags & SLOT_FREE)
			continue;
		membase_procs[nprocs].p = p;

		len = p->p_memmap[S].mem_vir - p->p_memmap[D].mem_vir + p->p_memmap[S].mem_len;

		/* is the text segment contiguous with the data segment? */
		if (p->p_memmap[T].mem_phys + p->p_memmap[T].mem_len == p->p_memmap[D].mem_phys) {
			base = p->p_memmap[T].mem_phys;
			len += p->p_memmap[T].mem_len;
		} else {
			base = p->p_memmap[D].mem_phys;
		}
		membase_procs[nprocs].base = (base << CLICK_SHIFT) >> 10; /* in K */
		membase_procs[nprocs].len = (len << CLICK_SHIFT) >> 10;

		if (p->p_nr < 0)
			mp = NULL;
		else
			mp = &mproc[p->p_nr];
		membase_procs[nprocs].mp = mp;
		nprocs++;
	}

	/* compact the list of holes */
	last_hole = nholes = -1;
	for (hi = 0; hi < _NR_HOLES; hi++) {
		h = &pmi->pmi_holes[hi];
                if (!h->h_base || !h->h_len)
			continue;
		nholes++;
		if (nholes != hi) {
			pmi->pmi_holes[nholes] = *h;
			last_hole = hi;
		}
	}

	if (last_hole != nholes)
		printf("warning: %d pseudo-holes were removed\n", last_hole - nholes);

	qsort(membase_procs, nprocs, sizeof(membase_procs[0]), cmp_base);
	qsort(pmi->pmi_holes, nholes, sizeof(pmi->pmi_holes[0]), cmp_h_base);

	printf("  PID COMMAND PRI    BEGIN      END     SIZE\n");
	hi = 0;
	pi = NR_TASKS;
	while (pi < nprocs || hi < nholes) {
		long h_base, h_len;

		if (pi < nprocs)
			base = membase_procs[pi].base;
		if (hi < nholes)
			h_base = (pmi->pmi_holes[hi].h_base << CLICK_SHIFT) >> 10;

		/* we arrived at the next hole */
		if (pi >= nprocs || (hi < nholes && base > h_base)) {
			h_len = (pmi->pmi_holes[hi].h_len << CLICK_SHIFT) >> 10;
			printf(" === [ hole ] === %8ldK %8ldK %8ldK\n",
				h_base,
				h_base + h_len,
				h_len);
			hi++;
			continue;
		}

		p = membase_procs[pi].p;
		mp = membase_procs[pi].mp;
		if(mp != NULL)
			printf("%5d %-8.8s", mp->mp_pid, mp->mp_name);
		else
			printf("[%3d] %-8.8s", p->p_nr, p->p_name);

		len = membase_procs[pi].len;
		printf(" %2d %8ldK %8ldK %8ldK\n",
			p->p_priority,
			base,
			base + len,
			len);
		pi++;
	}
}

int main(int argc, char *argv[])
{
	struct proc proc[PROCS];
        struct pm_mem_info pmi;
	struct mproc mproc[NR_PROCS];

        if(getsysinfo(PM_PROC_NR, SI_MEM_ALLOC, &pmi) < 0) {
		fprintf(stderr, "getsysinfo() for SI_MEM_ALLOC failed.\n");
		exit(1);;
	}

	if(getsysinfo(PM_PROC_NR, SI_KPROC_TAB, proc) < 0) {
		fprintf(stderr, "getsysinfo() for SI_KPROC_TAB failed.\n");
		exit(1);
	}

	if(getsysinfo(PM_PROC_NR, SI_PROC_TAB, mproc) < 0) {
		fprintf(stderr, "getsysinfo() for SI_PROC_TAB failed.\n");
		exit(1);
	}

	print_mem(proc, mproc, &pmi);

	return 0;
}

