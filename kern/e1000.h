#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H
#include <kern/pci.h>

/* General Constants */
#define E1000_MAX_PACKET_SIZE (1518)
#define E1000_TX_DESC (64)

#define E1000_VENDID (0x8086)
#define E1000_DEVID (0x100E)

/* Register Set */
#define E1000_CTRL     (0x00000 >> 2)  /* Device Control - RW */
#define E1000_STATUS   (0x00008 >> 2)  /* Device Status - RO */
#define E1000_TDBAL    (0x03800 >> 2)  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH    (0x03804 >> 2)  /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN    (0x03808 >> 2)  /* TX Descriptor Length - RW */
#define E1000_TDH      (0x03810 >> 2)  /* TX Descriptor Head - RW */
#define E1000_TDT      (0x03818 >> 2)  /* TX Descripotr Tail - RW */
#define E1000_TIDV     (0x03820 >> 2)  /* TX Interrupt Delay Value - RW */
#define E1000_TCTL     (0x00400 >> 2)  /* TX Control - RW */
#define E1000_TIPG     (0x00410 >> 2)  /* TX Inter-packet gap -RW */

/* Register Bit Masks */
/* Device Control */
#define E1000_CTRL_FD       0x00000001  /* Full duplex.0=half; 1=full */
#define E1000_CTRL_BEM      0x00000002  /* Endian Mode.0=little,1=big */
#define E1000_CTRL_PRIOR    0x00000004  /* Priority on PCI. 0=rx,1=fair */
#define E1000_CTRL_LRST     0x00000008  /* Link reset. 0=normal,1=reset */
#define E1000_CTRL_TME      0x00000010  /* Test mode. 0=normal,1=test */

/* Device Status */
#define E1000_STATUS_FD         0x00000001      /* Full duplex.0=half,1=full */
#define E1000_STATUS_LU         0x00000002      /* Link up.0=no,1=link */

/* Tramsmit Control */
#define E1000_TCTL_EN       0x00000002      /* enable tx */
#define E1000_TCTL_PSP      0x00000008      /* pad short packets */
#define E1000_TCTL_CT       0x00000ff0      /* collision threshold */
#define E1000_TCTL_COLD     0x003ff000      /* collision distance */

/* Transmit Descriptor Flags */
#define E1000_TXD_STAT_DD   0x00000001      /* Descriptor Done */
#define E1000_TXD_CMD_RS    0x00000008      /* Report Status */
#define E1000_TXD_CMD_EOP   0x00000001      /* End of Packet */

/* Transmit Descriptor Control Defaults */
#define E1000_TCTL_CT_OFFSET        (0x4)
#define E1000_TCTL_COLD_OFFSET      (0x12)
#define E1000_TCTL_CT_DEFAULT       (0x10 << E1000_TCTL_CT_OFFSET)
#define E1000_TCTL_COLD_DEFAULT     (0x40 << E1000_TCTL_COLD_OFFSET)

/* Inter-packet Gap Defaults */
#define E1000_IPGT  (0xA)
#define E1000_IPGR1 (0x4 << 10)
#define E1000_IPGR2 (0x6 << 20)

struct tx_desc {
  uint64_t addr;
  uint16_t length;
  uint8_t cso;
  uint8_t cmd;
  uint8_t status;
  uint8_t css;
  uint16_t special;
} __attribute__((packed));

struct tx_pkt {
  uint8_t buf[E1000_MAX_PACKET_SIZE];
} __attribute__((packed));

volatile uint32_t *e1000;

int e1000_attach(struct pci_func *);
int e1000_transmit(void *, uint32_t);

#endif	// JOS_KERN_E1000_H
