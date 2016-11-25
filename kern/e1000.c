#include <kern/e1000.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here
int
e1000_attach(struct pci_func *pcif) {

  // Enable PCI device
  pci_func_enable(pcif);

  // Map into memory
  e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
  assert(e1000[E1000_STATUS] == 0x80080783);
  cprintf("e1000 status: %x\n", e1000[E1000_STATUS]);



  return 0;
}
