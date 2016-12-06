#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H
#include <kern/pci.h>

/* General Constants */
#define E1000_MAX_PACKET_SIZE (1518)
#define E1000_TX_PACKET_SIZE (1518)
#define E1000_TX_DESC (64)
#define E1000_RX_PACKET_SIZE (2048)
#define E1000_RX_DESC (128)

#define E1000_VENDID (0x8086)
#define E1000_DEVID (0x100E)

/* Device Registers */
#define E1000_CTRL     (0x00000 >> 2)  /* Device Control - RW */
#define E1000_STATUS   (0x00008 >> 2)  /* Device Status - RO */

/* Device Control Flags */
#define E1000_CTRL_FD       0x00000001  /* Full duplex.0=half; 1=full */
#define E1000_CTRL_BEM      0x00000002  /* Endian Mode.0=little,1=big */
#define E1000_CTRL_PRIOR    0x00000004  /* Priority on PCI. 0=rx,1=fair */
#define E1000_CTRL_LRST     0x00000008  /* Link reset. 0=normal,1=reset */
#define E1000_CTRL_TME      0x00000010  /* Test mode. 0=normal,1=test */

/* Device Status Flags */
#define E1000_STATUS_FD         0x00000001      /* Full duplex.0=half,1=full */
#define E1000_STATUS_LU         0x00000002      /* Link up.0=no,1=link */


/* ******************** TRANSMITING ******************** */

/* Transmit Registers */
#define E1000_TDBAL    (0x03800 >> 2)  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH    (0x03804 >> 2)  /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN    (0x03808 >> 2)  /* TX Descriptor Length - RW */
#define E1000_TDH      (0x03810 >> 2)  /* TX Descriptor Head - RW */
#define E1000_TDT      (0x03818 >> 2)  /* TX Descripotr Tail - RW */
#define E1000_TIDV     (0x03820 >> 2)  /* TX Interrupt Delay Value - RW */
#define E1000_TCTL     (0x00400 >> 2)  /* TX Control - RW */
#define E1000_TIPG     (0x00410 >> 2)  /* TX Inter-packet gap -RW */

/* Tramsmit Control Flags */
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

/* Transmit Inter-packet Gap Defaults */
#define E1000_IPGT  (0xA)
#define E1000_IPGR1 (0x4 << 10)
#define E1000_IPGR2 (0x6 << 20)

/* Transmit Descriptor Struct */
struct tx_desc {
  uint64_t addr;
  uint16_t length;
  uint8_t cso;
  uint8_t cmd;
  uint8_t status;
  uint8_t css;
  uint16_t special;
} __attribute__((packed));


/* ******************** RECEIVING ******************** */

/* Receiving Registers */
#define E1000_RCTL     (0x00100 >> 2)  /* RX Control - RW */
#define E1000_RDBAL    (0x02800 >> 2)  /* RX Descriptor Base Address Low - RW */
#define E1000_RDBAH    (0x02804 >> 2)  /* RX Descriptor Base Address High - RW */
#define E1000_RDLEN    (0x02808 >> 2)  /* RX Descriptor Length - RW */
#define E1000_RDH      (0x02810 >> 2)  /* RX Descriptor Head - RW */
#define E1000_RDT      (0x02818 >> 2)  /* RX Descriptor Tail - RW */
#define E1000_RAL      (0x05400 >> 2)  /* Receive Address Low - RW Array */
#define E1000_RAH      (0x05404 >> 2)  /* Receive Address High - RW Array */

/* Receiving Control Flags */
#define E1000_RCTL_EN             0x00000002    /* enable */
#define E1000_RCTL_SBP            0x00000004    /* store bad packet */
#define E1000_RCTL_UPE            0x00000008    /* unicast promiscuous enable */
#define E1000_RCTL_MPE            0x00000010    /* multicast promiscuous enab */
#define E1000_RCTL_BSEX           0x02000000    /* Buffer size extension */
#define E1000_RCTL_SECRC          0x04000000    /* Strip Ethernet CRC */
#define E1000_RCTL_BAM            0x00008000    /* broadcast enable */

#define E1000_RCTL_LPE            0x00000020    /* long packet enable */
#define E1000_RCTL_LBM            0x000000C0    /* loopback mode */
#define E1000_RCTL_RDMTS          0x00000300    /* rx min threshold size */
#define E1000_RCTL_MO             0x00003000    /* multicast offset shift */
#define E1000_RCTL_SZ             0x00030000    /* rx buffer size */

/*Receiving Address Flags */
#define E1000_RAH_AV  0x80000000        /* Receive descriptor valid */

/* Receiving Descriptor Flags */
#define E1000_RXD_STAT_DD       0x01    /* Descriptor Done */
#define E1000_RXD_STAT_EOP      0x02    /* End of Packet */

/* Receiving Descriptor Struct */
struct rx_desc {
  uint64_t addr;        /* Address of the descriptor's data buffer */
  uint16_t length;      /* Length of data DMAed into data buffer */
  uint16_t csum;        /* Packet checksum */
  uint8_t status;       /* Descriptor status */
  uint8_t errors;       /* Descriptor Errors */
  uint16_t special;
} __attribute__((packed));

/* ******************** EERD ******************** */

/* EERD Register */
#define E1000_EERD     (0x00014 >> 2)  /* EEPROM Read - RW */

/* EERD Flags */
#define E1000_EEPROM_RW_REG_DATA   16   /* Offset to data in EEPROM read/write registers */
#define E1000_EEPROM_RW_REG_DONE   0x10 /* Offset to READ/WRITE done bit */
#define E1000_EEPROM_RW_REG_START  1    /* First bit for telling part to start operation */
#define E1000_EEPROM_RW_ADDR_SHIFT 8    /* Shift to the address bits */

volatile uint32_t *e1000;

int e1000_attach(struct pci_func *);
int e1000_transmit(void *, uint32_t);
int e1000_receive(void *);

/* Structure for MAC Address */
struct MAC {
  uint16_t high;
  uint32_t low;
};

#endif	// JOS_KERN_E1000_H
