#include <inc/types.h>
#define PCI_CLASS_DISK     0x01
#define PCI_CLASS_BRIDGE   0x06
#define PCI_CLASS_NIC      0x02


// for disk class
#define PCI_DISK_IDE 0x01

// for Bridge class
#define PCI_BDGE_RC  0x00



// for NIC
#define PCI_NIC_ETH 0x00





#define PCI_SUBCLASS_OTHER 0x80




struct pci_struct{
    uint8_t bus;
    uint8_t slot;
    uint8_t function;
};
extern struct pci_struct ide_bus_master;

struct busmaster_desc{
    unsigned command    : 8;
    unsigned dummy      : 8;
    unsigned status     : 8;
    unsigned dummy1     : 8;
    unsigned prdt_base : 32;
};


struct prdtbl{
    unsigned phy_addr : 32;
    unsigned size     : 16;
    unsigned dummy    : 15;
    unsigned eot      :  1;
};
#define PRDT_EOT 0x1
#define __PRDBASE 0x0660
#define PRDBASE ((__volatile__ struct prdtbl *) __PRDBASE)


#define BUS_MASTER_READ  0x00
#define BUS_MASTER_WRITE 0x08
#define BUS_MASTER_START 0x01
#define BUS_MASTER_STOP  0x00
// default BUS_MASTER_START = 0xc040
#define BUS_MASTER_START 0x0A00 
#define BUS_MASTER_CMD   BUS_MASTER_START
#define BUS_MASTER_STAT  BUS_MASTER_START+2
#define BUS_MASTER_PRDT  BUS_MASTER_START+4

#define SET_BUS_MASTER(cmd, stat, addr)  (struct busmaster_desc)     \
     { ((cmd) & 0xFF), 0x00, ((stat) & 0xFF), 0x00, addr }
#define SET_PRDTABLE(addr, size, eot) (struct prdtbl)               \
     { (addr), (size), 0x0, ((eot) & 0x1)}


uint16_t pciConfigReadWord (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

void pciConfigWriteDWord (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t data);

uint8_t  getHeaderType(uint8_t bus, uint8_t slot, uint8_t func);
uint8_t  getBaseClass(uint8_t bus, uint8_t slot, uint8_t func);
uint8_t  getSubClass(uint8_t bus, uint8_t slot, uint8_t func);
uint8_t  getSecondaryBus(uint8_t bus, uint8_t slot, uint8_t func);
uint16_t getVendorID(uint8_t bus, uint8_t slot, uint8_t func);
uint16_t getDeviceID(uint8_t bus, uint8_t slot, uint8_t func);
uint32_t setBAR4(uint8_t bus, uint8_t slot, uint8_t func, uint32_t data);
uint32_t getBAR4(uint8_t bus, uint8_t slot, uint8_t func);

