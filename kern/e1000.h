#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>

#define E1000_VEND     0x8086  // Vendor ID for Intel 
#define E1000_DEV      0x100E  // Device ID for the e1000 Qemu, Bochs, and VirtualBox emmulated NICs

struct e1000_tx_desc {
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
};

#define E1000_NTXDESC 32
#define E1000_MAXPACK 1518

__attribute__((__aligned__(sizeof(struct e1000_tx_desc))))
static volatile struct e1000_tx_desc tx_descs[E1000_NTXDESC];
static volatile uint8_t tx_buf[E1000_NTXDESC][E1000_MAXPACK];

#define REG_TDBAL    0x03800  /* TX Descriptor Base Address Low - RW */
#define REG_TDBAH    0x03804  /* TX Descriptor Base Address High - RW */
#define REG_TDLEN    0x03808  /* TX Descriptor Length - RW */
#define REG_TDH      0x03810  /* TX Descriptor Head - RW */
#define REG_TDT      0x03818  /* TX Descripotr Tail - RW */
#define REG_TCTL     0x00400  /* TX Control - RW */
#define REG_TIPG     0x00410  /* TX Inter-packet gap -RW */

#define TCTL_EN_BIT         (1 << 1)
#define TCTL_PSP_BIT        (1 << 3)
#define TCTL_CT_SHIFT       4
#define TCTL_COLD_SHIFT     12

#define TIPG_IPGT_SHIFT     0
#define TIPG_IPGR1_SHIFT    10
#define TIPG_IPGR2_SHIFT    20

#define TX_CMD_EOP_BIT     (1 << 0)
#define TX_CMD_IFCS_BIT    (1 << 1)
#define TX_CMD_RS_BIT      (1 << 3)
#define TX_STA_DD_BIT      (1 << 0)

int pci_e1000_attach(struct pci_func *pcif);

#endif  // SOL >= 6
