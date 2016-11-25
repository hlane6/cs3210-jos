#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H
#include <kern/pci.h>

#define E1000_VENDID (0x8086)
#define E1000_DEVID (0x100E)

int e1000_attach(struct pci_func *);

#endif	// JOS_KERN_E1000_H
