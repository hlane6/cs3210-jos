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
#include <kern/trap.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE 80 // enough for one VGA text line


struct Command {
  const char *name;
  const char *desc;
  // return -1 to force monitor to exit
  int (*func)(int argc, char **argv, struct Trapframe * tf);
};

static struct Command commands[] = {
  { "help",      "Display this list of commands",        mon_help       },
  { "info-kern", "Display information about the kernel", mon_infokern   },
  { "backtrace", "Display a backtrace of the stack",     mon_backtrace  },
  {
    "showmapping",
    "Display physical mapping information for range of virtual address",
    mon_showmapping
  },
  {
    "perm",
    "Set, clear, or change permision for a page mapped at a given virtual address",
    mon_perm
  },
  {
    "dump",
    "Dump contents of memory for a given range of virtual addresses",
    mon_dump
  }
};

#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

struct Perm {
  const char *name;
  const char *desc;
  int value;
};

static struct Perm permissions[] = {
  { "PTE_W",    "Write",          PTE_W   },
  { "PTE_U",    "User",           PTE_U   },
  { "PTE_PWT",  "Write-Through",  PTE_PWT },
  { "PTE_PCD",  "Cache-Disable",  PTE_PCD },
  { "PTE_A",    "Accessed",       PTE_A   },
  { "PTE_D",    "Dirty",          PTE_D   },
  { "PTE_PS",   "Page Size",      PTE_PS  },
  { "PTE_G",    "Global",         PTE_G   }
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
  int i;

  for (i = 0; i < NCOMMANDS; i++)
    cprintf("%s - %s\n", commands[i].name, commands[i].desc);
  return 0;
}

int
mon_infokern(int argc, char **argv, struct Trapframe *tf)
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
    cprintf("Stack Trace:\n");

    uint32_t *parameters, *frame_pointer = (uint32_t *) read_ebp();
    uintptr_t eip;
    struct Eipdebuginfo eip_info;

    while (frame_pointer) {
        parameters = frame_pointer + 2;
        eip = (uintptr_t) *(frame_pointer + 1);

        debuginfo_eip(eip, &eip_info);

        cprintf("  ebp %x eip %x args %08x %08x %08x %08x %08x\n",
                frame_pointer,
                eip,
                *parameters,
                *(parameters + 1),
                *(parameters + 2),
                *(parameters + 3),
                *(parameters + 4));

        cprintf("\t%s:%d: %.*s+%d\n",
                eip_info.eip_file,
                eip_info.eip_line,
                eip_info.eip_fn_namelen,
                eip_info.eip_fn_name,
                -eip_info.eip_fn_addr + eip);

        frame_pointer = (uint32_t *) *frame_pointer;
    }

  return 0;
}

int
mon_showmapping(int argc, char **argv, struct Trapframe *tf)
{
  if (argc < 3) {
    cprintf("Usage: showmapping [start address] [end address]\n");
    return 0;
  }

  char *start, *end;

  start = (char *) ROUNDDOWN(strtol(argv[1], NULL, 16), PGSIZE);
  end = (char *) ROUNDDOWN(strtol(argv[2], NULL, 16), PGSIZE);

  if ((uintptr_t) start < KERNBASE || (uintptr_t) end < KERNBASE) {
    cprintf("Valid virtual addresses are required\n");
    return 0;
  }

  cprintf("va \t\t pa \t\t perm\n");

  for (char *cur = start; cur <= end; cur += PGSIZE) {
    pte_t *page_entry = pgdir_walk(kern_pgdir, cur, 0);
    if (*page_entry & PTE_P) {
      cprintf("%08p \t %08p \t %08p\n", cur, page_entry, PGOFF(page_entry));
    }
  }

  return 0;
}

int
mon_perm(int argc, char **argv, struct Trapframe *tf)
{
  if (argc < 3) {
    cprintf("Usage: perm [virtual address] [ s [perm] | r ]\n");
    return 0;
  }

  char *command, *addr;
  pte_t *pagetable_entry;
  int perm, i, j;

  command = argv[2];
  addr = (char *) strtol(argv[1], NULL, 16);
  perm = PTE_P;

  if ((uintptr_t) addr < KERNBASE) {
    cprintf("Valid virtual addresses are required\n");
    return 0;
  }

  switch (*command) {
    case 's':
      if (argc < 4) {
        cprintf("Need to specify what permissions to use\n");
        return 0;
      }

      for (i = 3; i < argc; i++)
        for (j = 0; j < PGFLAG; j++)
          if (!strcmp(permissions[j].name, argv[i]))
            perm |= permissions[j].value;

      if ( (pagetable_entry = pgdir_walk(kern_pgdir, addr, 0)) ) {
        *pagetable_entry = PTE_ADDR(*pagetable_entry) | perm;
        cprintf("Permissions successfully changed to %p\n", perm);
      } else {
        cprintf("Page not mapped, could not change permissions\n");
      }
      break;

    case 'r':
      if ( (pagetable_entry = pgdir_walk(kern_pgdir, addr, 0)) ) {
        *pagetable_entry = PTE_ADDR(*pagetable_entry) | perm;
        cprintf("Permissions successfully removed\n");
      } else {
        cprintf("Page not mapped, could not change permissions\n");
      }
      break;
  }

  return 0;
}

int
mon_dump(int argc, char **argv, struct Trapframe *tf)
{
  if (argc < 3) {
    cprintf("Usage: dump [start address] [end address]\n");
    return 0;
  }

  char *start, *end, phys;

  start = (char *) strtol(argv[1], NULL, 16);
  end = (char *) strtol(argv[2], NULL, 16);
  phys = 0;

  if ((uintptr_t) start < KERNBASE && (uintptr_t) end < KERNBASE) {
    phys = 1;
    start = KADDR((physaddr_t) start);
    end = KADDR((physaddr_t) end); 
  }

  cprintf("%s Address\t\tValue\n", (phys) ? "Physical" : "Virtual");

  for ( ; start < end; start++) {
    cprintf("%p: \t\t\t%p\n", (phys) ? PADDR(start) : (uintptr_t) start, *start);
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
  for (i = 0; i < NCOMMANDS; i++)
    if (strcmp(argv[0], commands[i].name) == 0)
      return commands[i].func(argc, argv, tf);
  cprintf("Unknown command '%s'\n", argv[0]);
  return 0;
}

void
monitor(struct Trapframe *tf)
{
  char *buf;

  cprintf("Welcome to the JOS kernel monitor!\n");
  cprintf("Type 'help' for a list of commands.\n");

<<<<<<< HEAD
  if (tf != NULL)
    print_trapframe(tf);

=======
>>>>>>> lab2
  while (1) {
    buf = readline("K> ");
    if (buf != NULL)
      if (runcmd(buf, tf) < 0)
        break;
  }
}
