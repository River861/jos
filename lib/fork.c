// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

extern volatile pte_t uvpt[];
extern volatile pde_t uvpd[];
extern void _pgfault_upcall(void);

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int res;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	addr = ROUNDDOWN(addr, PGSIZE);

	pde_t pde = uvpd[PDX(addr)];
	pte_t pte = uvpt[PGNUM(addr)];
	if (!((pde & PTE_P) && (pte & PTE_P) && (pte & PTE_U) && (pte & PTE_COW) && (err & FEC_WR)))
	{
        panic(
            "[%08x] user page fault va 0x%08x ip 0x%08x: [%s, %s, %s]",
            sys_getenvid(), utf->utf_fault_va, utf->utf_eip,
            err & 4 ? "user" : "kernel",
            err & 2 ? "write" : "read",
            err & 1 ? "protection" : "not-present"
        );
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	if ((res = sys_page_alloc(0, PFTEMP, PTE_P | PTE_U | PTE_W)) < 0)
	{
		panic("pgfault: %e", res);
	}
	memcpy((void *)PFTEMP, addr, PGSIZE);  // copy
	if ((res = sys_page_map(0, PFTEMP, 0, addr, PTE_P | PTE_U | PTE_W)) < 0)
	{
		panic("pgfault: %e", res);
	}
	if ((res = sys_page_unmap(0, PFTEMP)) < 0)
	{
		panic("pgfault: %e", res);
	}

	// panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int res;

	// LAB 4: Your code here.
	void *pg = (void *)(pn * PGSIZE);
	pte_t pte = uvpt[pn];

	assert((pte & PTE_P) && (pte & PTE_U));

	if (pte & PTE_SHARE)
	{
		if ((res = sys_page_map(0, pg, envid, pg, PTE_SYSCALL)) < 0)
		{
			return res;
		}
	}
	else if ((pte & PTE_W) || (pte & PTE_COW))
	{
		if ((res = sys_page_map(0, pg, envid, pg, PTE_P | PTE_U | PTE_COW)) < 0)
		{
			return res;
		}
		if ((res = sys_page_map(0, pg, 0, pg, PTE_P | PTE_U | PTE_COW)) < 0)  // parent mapping should be marked copy-on-write as well
		{
			return res;
		}
	}
	else
	{
		if ((res = sys_page_map(0, pg, envid, pg, PTE_P | PTE_U)) < 0)
		{
			return res;
		}
	}

	// panic("duppage not implemented");
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	set_pgfault_handler(pgfault);

	envid_t ceid;
	int res;

	if ((ceid = sys_exofork()) == 0)  // child
	{
		thisenv = &envs[ENVX(sys_getenvid())];
	}
	else  // parent
	{
		for (int page_idx = 0; page_idx < UTOP / PGSIZE - 1;)   // Xstack should not be mapped
		{
			uint32_t pde = uvpd[page_idx / NPDENTRIES];
			if (!(pde & PTE_P))
			{
				page_idx += NPDENTRIES;
			}
			else
			{
				uint32_t pt_end = MIN(UTOP / PGSIZE - 1, page_idx + NPDENTRIES);
				for(; page_idx < pt_end; ++ page_idx)
				{
					uint32_t pte = uvpt[page_idx];
					if ((pte & PTE_P) && (pte & PTE_U))
					{
						if ((res = duppage(ceid, page_idx)) < 0)
						{
							panic("fork: %e\n", res);
						}
					}
				}
			}
		}
		if ((res = sys_page_alloc(ceid, (void *)(UXSTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W)) < 0)  // Xstack
		{
			panic("fork: %e\n", res);
		}
		if ((res = sys_env_set_pgfault_upcall(ceid, _pgfault_upcall)) < 0)
		{
			panic("fork: %e\n", res);
		}
		if ((res = sys_env_set_status(ceid, ENV_RUNNABLE)) < 0)
		{
			panic("fork: %e\n", res);
		}
	}
	return ceid;

	// panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
