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

  // DEBUG
  cprintf("[ pgfault error ] [ %p %x ]\n", utf->utf_fault_va, err);

  // Check that the faulting access was (1) a write, and (2) to a
  // copy-on-write page.  If not, panic.
  // Hint:
  //   Use the read-only page table mappings at uvpt
  //   (see <inc/memlayout.h>).

  // LAB 4: Your code here.
  if ( !(err & FEC_WR) ) {
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
  addr = ROUNDDOWN(addr, PGSIZE);

  // map page to temporary location
  if ( (r = sys_page_alloc(0, PFTEMP, (PTE_P | PTE_U | PTE_W))) < 0) {
    panic("sys_page_alloc: %e", r);
  }

  // copy data from old page to new page
  memmove((void *) PFTEMP, addr, PGSIZE);

  // move new page to old page address
  if ( (r = sys_page_map(0, PFTEMP, 0, addr, (PTE_P | PTE_U | PTE_W))) < 0) {
    panic("sys_page_map: %e", r);
  }

  if ( (r = sys_page_unmap(0, PFTEMP)) < 0) {
    panic("sys_page_unmap: %e", r);
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
  void *addr = (void *) (pn << PGSHIFT);
  pte_t pte = uvpt[pn];

  // if page isnt COW or W, map in as is
  if ( !(pte & (PTE_W | PTE_COW)) ) {
    if ( (r = sys_page_map(0, addr, envid, addr, (pte & PTE_SYSCALL))) < 0) {
      panic("sys_page_map: %e", r);
    }
    return 0;
  }

  // otherwise copy into both as COW
  if ( (r = sys_page_map(0, addr, envid, addr, (PTE_COW | PTE_U | PTE_P))) < 0) {
    panic("sys_page_map: %e", r);
  }

  if ( (r = sys_page_map(0, addr, 0, addr, (PTE_COW | PTE_U | PTE_P))) < 0) {
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
  envid_t envid, parent_id;
  uint32_t i, j, pn, end;
  int error;

  parent_id = sys_getenvid();

  // set up page fault handler
  set_pgfault_handler(pgfault);

  // create a child with sys_exofork
  envid = sys_exofork();
  
  if (envid < 0) {
    panic("sys_exofork: %e", envid);
  } else if (envid == 0) {
    // we are the child
    thisenv = &envs[ENVX(sys_getenvid())];
    return 0;
  }

  // we are the parent
  for (pn = PGNUM(UTEXT); pn < PGNUM(UTOP); pn += NPTENTRIES) {
    if ( !(uvpd[pn >> (PDXSHIFT - PTXSHIFT)] & PTE_P) )
      continue;

    end = pn + NPTENTRIES;
    for ( ; pn < end; pn++) {
      if (pn == PGNUM(UXSTACKTOP - 1))
        continue;

      if ( !(uvpt[pn] & (PTE_P | PTE_U)) )
        continue;
      
      duppage(envid, pn);
    }
  }

  // allocate new page for child system
  if ( (error = sys_page_alloc(envid,
      (void *) (UXSTACKTOP - PGSIZE),
      (PTE_P | PTE_U | PTE_W))) < 0) {
    panic("sys_page_alloc: %e", error);
  }

  // set pgfault_upcall for child
  if ( (error = sys_env_set_pgfault_upcall(envid,
          thisenv->env_pgfault_upcall)) < 0) {
    panic("sys_env_set_pgfault_upcall: %e", error);
  }

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
