#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>
#include <inc/mmu.h>

// LAB 6: Your driver code here

// Variables
struct tx_desc tx_descs[E1000_TX_DESC] __attribute__((aligned (16)));
uint8_t tx_desc_bufs[E1000_TX_DESC][E1000_TX_PACKET_SIZE] __attribute__((aligned (16)));

struct rx_desc rx_descs[E1000_RX_DESC] __attribute__((aligned (16)));
uint8_t rx_desc_bufs[E1000_RX_DESC][E1000_RX_PACKET_SIZE] __attribute__((aligned (16)));

// Initialize e1000 transmitting functionality
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
static void
e1000_tx_init() {
  int i;

  memset(tx_descs, 0, sizeof(tx_descs));
  memset(tx_desc_bufs, 0, sizeof(tx_desc_bufs));

  for (i = 0; i < E1000_TX_DESC; i++) {
    tx_descs[i].addr = PADDR(&tx_desc_bufs[i]);
    tx_descs[i].status |= E1000_TXD_STAT_DD;
  }

  e1000[E1000_TDBAL] = PADDR(tx_descs);
  e1000[E1000_TDBAH] = 0x0;
  e1000[E1000_TDLEN] = sizeof(struct tx_desc) * E1000_TX_DESC;
  e1000[E1000_TDH] = 0x0;
  e1000[E1000_TDT] = 0x0;
  e1000[E1000_TCTL] &= (~E1000_TCTL_CT | ~E1000_TCTL_COLD);
  e1000[E1000_TCTL] |= (E1000_TCTL_EN | E1000_TCTL_PSP | E1000_TCTL_CT_DEFAULT | E1000_TCTL_COLD_DEFAULT);
  e1000[E1000_TIPG] = (E1000_IPGT | E1000_IPGR1 | E1000_IPGR2);
}

// Initialize e1000 receiving functionality
//
// 1.) Prgoram Receive Address (RAL / RAH) with MAC address
//     a.) Set address valid bit on RAH
// 2.) Fill RDBAL / RDBAH with receive descriptor array address
// 3.) Set receive descriptor length (RDLEN) register to size of queue
// 4.) Set RDH to 0
// 5.) Set RDT to 1 past the last valid rx descriptor (would this also be 0?)
// 6.) Program Receive Cotnrol register
//     a.) Set RCTL.EN bit to 1
//     b.) Don't set RCTL.LPE yet (mayber later to allow bigger packets)
//     c.) Set RCTL.LBM to 0x00
//     d.) Set RCTL.BSIZE to accept packets of 2048
//     e.) Set RCTL.SECRC to 1 to strip CRC 
static void
e1000_rx_init() {
  int i;

  memset(rx_descs, 0, sizeof(rx_descs));
  memset(rx_desc_bufs, 0, sizeof(rx_desc_bufs));

  for (i = 0; i < E1000_RX_DESC; i++) {
    rx_descs[i].addr = PADDR(&rx_desc_bufs[i]);
  }

  e1000[E1000_RAL] = 0x12005452;
  e1000[E1000_RAH] = (0x5634 | E1000_RAH_AV);

  e1000[E1000_RDBAL] = PADDR(rx_descs);
  e1000[E1000_RDBAH] = 0x0;

  e1000[E1000_RDLEN] = sizeof(rx_descs);

  e1000[E1000_RDH] = 0x0;
  e1000[E1000_RDT] = E1000_RX_DESC;

  e1000[E1000_RCTL] = (E1000_RCTL_EN | E1000_RCTL_SECRC);
}

int
e1000_attach(struct pci_func *pcif) {

  // Enable PCI device
  pci_func_enable(pcif);

  // Map into memory
  e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
  assert(e1000[E1000_STATUS] == 0x80080783);

  e1000_tx_init();
  e1000_rx_init();

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
  if (len >= E1000_TX_PACKET_SIZE)
    return -1;

  // Get current tail
  uint32_t tx_indx = e1000[E1000_TDT];

  // Check if next descriptor is free by checking if dd is set in
  // the status field of the descriptor
  if ( !(tx_descs[tx_indx].status & E1000_TXD_STAT_DD) )
    return -1;
  
  // Transmit a packet
  // 1.) Add to tail of trasmit queue
  //     a.) Copy packet data into descriptor's buffer
  //     b.) Set RS bit in command field of transmit descriptor
  // 2.) Update tail of queue (TDT register)
  memmove(tx_desc_bufs[tx_indx], packet, len);
  tx_descs[tx_indx].length = len;

  tx_descs[tx_indx].status &= ~E1000_TXD_STAT_DD;
  tx_descs[tx_indx].cmd = (E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP);    

  e1000[E1000_TDT] = (e1000[E1000_TDT] + 1) % E1000_TX_DESC;

  return 0;
}

/**
 * Attempts to receive a packet by taking the tail
 * and seeing if it is valid, moving the packet, and 
 * then updating the tail
 */
int
e1000_receive(void *buf) {
  // Receive a packet
  // 1.) Get next available descriptor
  // 2.) Copy data into buffer
  // 3.) Unset DD
  // 4.) Update tail
  //
  uint32_t rx_indx = e1000[E1000_RDT];
  cprintf("receving\n");

  if ( !(rx_descs[rx_indx].status & E1000_RXD_STAT_DD) )
    return -1;
  cprintf("still receving\n");

  memmove(buf, rx_desc_bufs[rx_indx], rx_descs[rx_indx].length);
  rx_descs[rx_indx].status &= ~(E1000_RXD_STAT_DD | E1000_RXD_STAT_EOP);

  e1000[E1000_RDT] = (e1000[E1000_RDT] + 1) % E1000_RX_DESC;

  return 0;

}
