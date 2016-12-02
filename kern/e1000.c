#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>
#include <inc/mmu.h>

// LAB 6: Your driver code here

// Variables
struct tx_desc tx_desc_array[E1000_TX_DESC] __attribute__((aligned (16)));
struct tx_pkt tx_desc_bufs[E1000_TX_DESC] __attribute__((aligned (16)));

int
e1000_attach(struct pci_func *pcif) {

  // Enable PCI device
  pci_func_enable(pcif);

  // Map into memory
  e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
  assert(e1000[E1000_STATUS] == 0x80080783);
  cprintf("e1000 status: %x\n", e1000[E1000_STATUS]);

  // Allocate descriptor array, buffers
  memset(tx_desc_array, 0, sizeof(struct tx_desc) * E1000_TX_DESC);
  memset(tx_desc_bufs, 0, sizeof(struct tx_pkt) * E1000_TX_DESC);

  int i;
  for (i = 0; i < E1000_TX_DESC; i++) {
    // Set buffer address of every transmit buffer
    // Set every descriptor as E1000_TXD_STAT_DD
    tx_desc_array[i].addr = PADDR(&tx_desc_bufs[i]);
    tx_desc_array[i].status |= E1000_TXD_STAT_DD;
  }

  // Initialize e1000
  //
  // 1.) Fill TDBAL / TBBAH with to base address of transmit
  //     descriptor array
  // 2.) Set TDLEN to length of transmit descriptor array
  //     (needs to be 128 byte aligned)
  // 3.) Write 0 to TDH and TDT registers
  // 4.) Initialize TCTL register with these values:
  //     a.) TCTL.EN bit to 0x1
  //     b.) TCTL.PSP bit to 1
  //     c.) TCTL.CT to 0x10
  //     d.) TCTL.COLD to 0x40
  // 5.) Initialize TIPG
  //
  e1000[E1000_TDBAL] = PADDR(tx_desc_array);
  e1000[E1000_TDBAH] = 0x0;

  e1000[E1000_TDLEN] = sizeof(struct tx_desc) * E1000_TX_DESC;

  e1000[E1000_TDH] = 0x0;
  e1000[E1000_TDT] = 0x0;

  e1000[E1000_TXDCTL] |= E1000_TCTL_EN;
  e1000[E1000_TXDCTL] |= E1000_TCTL_PSP;
  e1000[E1000_TXDCTL] &= ~E1000_TCTL_CT;
  e1000[E1000_TXDCTL] |= E1000_TCTL_CT_DEFAULT;
  e1000[E1000_TXDCTL] &= ~E1000_TCTL_COLD;
  e1000[E1000_TXDCTL] |= E1000_TCTL_COLD_DEFAULT;

  e1000[E1000_TIPG] = 0xa;
  e1000[E1000_TIPG] |= (E1000_IPGT | E1000_IPGR1 | E1000_IPGR2);

  cprintf("[ bal tdlen tdh tdxctl tipg ]\n");
  cprintf("[ %x %x %x %x %x %x ]\n", e1000[E1000_TDBAL],
      e1000[E1000_TDLEN],
      e1000[E1000_TDH],
      e1000[E1000_TXDCTL],
      (E1000_IPGT | E1000_IPGR1 | E1000_IPGR2));

  uint8_t pkt[] = {1, 2, 0xa, 4, 5};
  e1000_transmit(pkt, 5);

  return 0;
}

/**
 * Attempts to transmit a packet by placing it at the end
 * of e1000's transmit queue.
 * Errors:
 *   trying to send too large of a packet
 *   transmit queue is full
 * Returns 0 on success
 * Returns <0 on error
 */
int
e1000_transmit(void *packet, uint32_t len) {
  if (len >= E1000_MAX_PACKET_SIZE)
    return -1;

  // Get current tail
  uint32_t new_tail_index = (e1000[E1000_TDT] + 1) % E1000_TX_DESC;
  struct tx_desc *new_tail = &tx_desc_array[new_tail_index]; 

  // Check if next descriptor is free by checking if dd is set in
  // the status field of the descriptor
  if ( !(new_tail->status & E1000_TXD_STAT_DD) )
    return -1;
  
  // Transmit a packet
  // 1.) Add to tail of trasmit queue
  //     a.) Copy packet data into descriptor's buffer
  //     b.) Set RS bit in command field of transmit descriptor
  // 2.) Update tail of queue (TDT register)
  memmove(tx_desc_bufs[new_tail_index].buf, packet, len);
  new_tail->length = len;

  new_tail->cmd |= E1000_TXD_CMD_RS;    
  new_tail->status &= ~E1000_TXD_STAT_DD;
  new_tail->status |= E1000_TXD_CMD_EOP; 

  cprintf("prev tail: %d\n", e1000[E1000_TDT]);
  e1000[E1000_TDT] = new_tail_index;
  cprintf("new tail: %d\n", e1000[E1000_TDT]);

  return 0;
}
