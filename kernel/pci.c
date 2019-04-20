#include <kernel/trap.h>
#include <kernel/picirq.h>
#include <inc/stdio.h>
#include <inc/types.h>
#include <inc/x86.h>
#include <inc/pci.h>
// #include <inc/pci_generic.h>
#include <kernel/pci_dev.h>
void pci_start_disk(uint8_t, uint8_t, uint8_t, uint8_t);
void pci_unknow_class(void) {
    return;
}


struct pci_struct ide_bus_master;


uint16_t pciConfigReadWord (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint16_t tmp = 0;
 
    /* create configuration address as per Figure 1 */
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
 
    /* write out the address */
    outl(0xCF8, address);
    /* read in the data */
    /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
    tmp = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
    // cprintf("\t\t[%d/%d/%d: %lx]------->%lx\n",bus,slot,func,offset,tmp);
    return (tmp);
}

void pciConfigWriteDWord (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t data) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
 
    /* create configuration address as per Figure 1 */
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
 
    /* write out the address */
    outl(0xCF8, address);
    outl(0xCFC, data);
    /* read in the data */
    // cprintf("\t\t[%d/%d/%d: %lx]------->%lx\n",bus,slot,func,offset,tmp);
    return;
}

uint8_t  getHeaderType(uint8_t bus, uint8_t slot, uint8_t func){
    return (uint8_t) (pciConfigReadWord(bus, slot, func, 0xE) & 0xFF);
}
uint8_t  getBaseClass(uint8_t bus, uint8_t slot, uint8_t func){
    return (uint8_t) (pciConfigReadWord(bus, slot, func, 0xA) >> 8);
}
uint8_t  getSubClass(uint8_t bus, uint8_t slot, uint8_t func){
    return (uint8_t) (pciConfigReadWord(bus, slot, func, 0xA) & 0xFF);
}
uint8_t  getSecondaryBus(uint8_t bus, uint8_t slot, uint8_t func){
    return (uint8_t) (pciConfigReadWord(bus, slot, func, 0x18) >> 8);
}
uint16_t getVendorID(uint8_t bus, uint8_t slot, uint8_t func){
    return pciConfigReadWord(bus, slot, func, 0);
}
uint16_t getDeviceID(uint8_t bus, uint8_t slot, uint8_t func){
    return pciConfigReadWord(bus, slot, func, 2);
}
uint32_t setBAR4(uint8_t bus, uint8_t slot, uint8_t func, uint32_t data){
    uint32_t ret;
    pciConfigWriteDWord(bus, slot, func, 0x20, data);
    ret = getBAR4(bus, slot, func);
    return ret;
}
uint32_t getBAR4(uint8_t bus, uint8_t slot, uint8_t func){
    uint32_t ret;
    ret =  pciConfigReadWord(bus, slot, func, 0x22) << 16;
    ret |= pciConfigReadWord(bus, slot, func, 0x20);
    return ret;
}


void checkFunction(uint8_t bus, uint8_t device, uint8_t function) {
    uint8_t baseClass;
    uint8_t subClass;
    uint16_t deviceID, vendorID;
    uint8_t secondaryBus;

    baseClass = getBaseClass(bus, device, function);
    subClass = getSubClass(bus, device, function);
    if( (baseClass == 0x06) && (subClass == 0x04) ) {
        secondaryBus = getSecondaryBus(bus, device, function);
        cprintf("\t is a secondaryBus, bus=%d\n", secondaryBus);
        checkBus(secondaryBus);
    }else{
        vendorID = getVendorID(bus, device, function);
        deviceID = getDeviceID(bus, device, function);
        cprintf("(%lx,%lx)--> Func = %d, baseClass = %lx, subClass = %lx\n",
                 vendorID, deviceID, function, baseClass, subClass);
        switch (baseClass) {
            case PCI_CLASS_DISK:
                pci_start_disk(bus, device, function, subClass);
                break;
            default:
                pci_unknow_class();
        }
    }
}

void checkDevice(uint8_t bus, uint8_t device) {
    uint8_t function = 0;
    uint8_t headerType;
    uint16_t deviceID;
    uint16_t vendorID;
    
    vendorID = getVendorID(bus, device, function);
    deviceID = getDeviceID(bus, device, function);
    if(vendorID == 0xFFFF) return;        // Device doesn't exist
    cprintf("bus = %d, slot = %d, function = %d, vendor = %lx, device = %lx\n",
            bus, device, function, vendorID, deviceID);
    checkFunction(bus, device, function);
    headerType = getHeaderType(bus, device, function);
    if( (headerType & 0x80) != 0) {
        /* It is a multi-function device, so check remaining functions */
        for(function = 1; function < 8; function++) {
            if(getVendorID(bus, device, function) != 0xFFFF) {
                checkFunction(bus, device, function);
            }
        }
    }
}

void checkBus(uint8_t bus) {
    uint8_t device;

    for(device = 0; device < 32; device++) {
        checkDevice(bus, device);
    }
}

void checkAllBuses(void) {
    uint8_t function;
    uint8_t bus;
    uint8_t headerType;
    
    headerType = getHeaderType(0, 0, 0);
    if( (headerType & 0x80) == 0) {
        /* Single PCI host controller */
        checkBus(0);
    } else {
        /* Multiple PCI host controllers */
        for(function = 0; function < 8; function++) {
            if(getVendorID(0, 0, function) != 0xFFFF) break;
            bus = function;
            checkBus(bus);
        }
    }
}

void pci_init(void){
    checkAllBuses();
}

// Disk (DMA) controller 



void pci_start_disk(uint8_t bus, uint8_t slot, uint8_t func, uint8_t subClass){
    cprintf("\t\tInitializing disk controller@[%d/%d/%d]\n", 
            bus, slot, func);

    // we implement PIIX 3 only
    if (getDeviceID(bus, slot, func) == 0x7010) {
        irq_setmask_8259A(irq_mask_8259A & ~(1<<IRQ_IDE));
        ide_bus_master.bus      = bus;
        ide_bus_master.slot     = slot;
        ide_bus_master.function = func;
        ide_init(); 
    }
    
    return;
}

