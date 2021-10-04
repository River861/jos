// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
<<<<<<< HEAD
#include <kern/trap.h>
=======
#include <kern/pmap.h>
>>>>>>> lab2

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "showmappings", "Display page mappings", mon_showmap },
	{ "showvmem", "Display the content at virtual memory", mon_showvmem }
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
	return 0;
}

int
mon_showmap(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t vstart, vend;

	vstart = (uint32_t) strtol(argv[1], 0, 16);
	vend = (uint32_t) strtol(argv[2], 0, 16);

	vstart = ROUNDDOWN(vstart, PGSIZE);
	vend = ROUNDDOWN(vend, PGSIZE);

	for(uint32_t va = vstart; va <= vend; va += PGSIZE) {
		pte_t *pte = pgdir_walk(kern_pgdir, (void*)va, 0);
		if (pte && (*pte & PTE_P))
		{
			cprintf("VA: 0x%08x, PA: 0x%08x, U-bit: %d, W-bit: %d\n", va, PTE_ADDR(*pte), !!(*pte & PTE_U), !!(*pte & PTE_W));
		} else
		{
			cprintf("VA: 0x%08x, PA: No Mapping\n", va);
		}
	}
	return 0;
}

int
mon_showvmem(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t vstart, vend;

	vstart = (uint32_t) strtol(argv[1], 0, 16);
	vend = (uint32_t) strtol(argv[2], 0, 16);

	for(uint32_t va = vstart; va <= vend; ++ va) {
		pte_t *pte = pgdir_walk(kern_pgdir, (void*)va, 0);
		if (pte && (*pte & PTE_P))
		{
			cprintf("[VA: 0x%08x, PA: 0x%08x]: %02x\n", va, PTE_ADDR(*pte) | PGOFF(va), *((uint8_t*)(va)));
		} else
		{
			cprintf("[VA: 0x%08x, PA: No Mapping]: --\n", va);
		}
	}
	return 0;
}


/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
