// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW         0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
  void *addr = (void*)utf->utf_fault_va;
  uint32_t err = utf->utf_err;
  int r;

  // Check that the faulting access was (1) a write, and (2) to a
  // copy-on-write page.  If not, panic.
  // Hint:
  //   Use the read-only page table mappings at uvpt
  //   (see <inc/memlayout.h>).

  // LAB 4: Your code here.
  if (err == FEC_WR) {
    panic("user_pgfault: %e", err);
  }

  if ( !(uvpt[PGNUM(addr)] & PTE_COW) ) {
    panic("user_pgfault: page not COW");
  }

  // Allocate a new page, map it at a temporary location (PFTEMP),
  // copy the data from the old page to the new page, then move the new
  // page to the old page's address.
  // Hint:
  //   You should make three system calls.

  // LAB 4: Your code here.
  // map page to temporary location
  if ( (r = sys_page_alloc(sys_getenvid(),
     (void *) (PFTEMP),
     (PTE_P | PTE_U | PTE_W))) < 0) {
    panic("sys_page_alloc: %e", r);
  }

  // copy data from old page to new page
  memmove((void *) PFTEMP, ROUNDDOWN(addr, PGSIZE), PGSIZE);

  // move new page to old page address
  if ( (r = sys_page_map(sys_getenvid(),
      (void *) (PFTEMP),
      sys_getenvid(),
      (void *) ROUNDDOWN(addr, PGSIZE),
      (PTE_P | PTE_U | PTE_W))) < 0) {
    panic("sys_page_map: %e", r);
  }
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
  int r;

  // LAB 4: Your code here.
  // map page into child address space as copy on write
  if ( (r = sys_page_map(0,
      (void *) (pn * PGSIZE),
      envid,
      (void *) (pn * PGSIZE),
      (PTE_COW | PTE_U | PTE_P))) < 0) {
    panic("sys_page_map: %e", r);
  }

  // map page into own address space as copy on write
  if ( (r = sys_page_map(0,
      (void *) (pn * PGSIZE),
      0,
      (void *) (pn * PGSIZE),
      (PTE_COW | PTE_U | PTE_P))) < 0) {
    panic("sys_page_map: %e", r);
  } 

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
  envid_t envid;
  uint32_t i, j, pn;
  int error;

  // set up page fault handler
  set_pgfault_handler(pgfault);

  // create a child with sys_exofork
  envid = sys_exofork();
  
  if (envid < 0) {
    panic("sys_exofork: %e", envid);
  } else if (envid == 0) {
    // we are the child
    thisenv = &envs[ENVX(sys_getenvid())];
    set_pgfault_handler(pgfault);
    return 0;
  }

  // we are the parent
  for (i = 0; i < PDX(UTOP); i++) {
    if (uvpd[i] & PTE_P) {
      for (j = 0; j < NPTENTRIES; j++) {
        pn = i * NPTENTRIES + j;

        if ((pn != PGNUM(UXSTACKTOP - PGSIZE)) &&
            (uvpt[pn] & PTE_P)) {
          duppage(envid, pn);
        }
      }
    }
  }

  // allocate new page for child system
  if ( (error = sys_page_alloc(envid,
      (void *) (UXSTACKTOP - PGSIZE),
      (PTE_P | PTE_U | PTE_W))) < 0) {
    panic("sys_page_alloc: %e", error);
  }

  if ( (error = sys_page_map(envid,
      (void *) (UXSTACKTOP - PGSIZE),
      sys_getenvid(),
      (void *) (PFTEMP),
      (PTE_P | PTE_U | PTE_W))) < 0) {
    panic("sys_page_map: %e", error);
  }

  memmove((void *) (UXSTACKTOP - PGSIZE),
      (void *) (PFTEMP),
      PGSIZE);

  // set pgfault_upcall for child
  sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall);

  if ( (error = sys_env_set_status(envid, ENV_RUNNABLE)) < 0) {
    panic("sys_env_set_status: %e", error);
  }

  return envid;
}

// Challenge!
int
sfork(void)
{
  panic("sfork not implemented");
  return -E_INVAL;
}
