#include <kernel/pci_dev.h>
#include <kernel/trap.h>
#include <inc/stdio.h>
#include <inc/x86.h>

static uint8_t bus, slot, func; 
void ide_init(void){
    bus  = ide_bus_master.bus;
    slot = ide_bus_master.slot;
    func = ide_bus_master.function;
    return;
}

void ide_intr(struct Trapframe *tf){
    uint8_t stat = inb(BUS_MASTER_STAT);
    if (stat == 0x4) {
        cprintf("CATCHING DMA IRQ\n");
        outb(BUS_MASTER_CMD,0x0); // command stop
        outb(BUS_MASTER_STAT, 0x6); // clear status and intr bits
        cprintf("DMA transfer complete, now (%lx, %lx)\n", 
                inb(BUS_MASTER_STAT), inb(BUS_MASTER_CMD));
    }else{
        cprintf("DMA encounters errors");
        cprintf("ATA primary error %lx\n", inb(0x1F1));
        cprintf("ATA status: %lx\n", inb(0x1F7));
        cprintf("*BAR4 status: %lx\n", inb(BUS_MASTER_STAT) );
        cprintf("*BAR4 command: %lx\n", inb(BUS_MASTER_CMD));
        cprintf("*BAR4 prdt_bse: %lx\n", inl(BUS_MASTER_PRDT));
        cprintf("pci command: %lx\n", pciConfigReadWord(bus, slot, func, 0x4));
        cprintf("pci status : %lx\n", pciConfigReadWord(bus, slot, func, 0x6));
        cprintf("pci IDETIM : %lx\n", pciConfigReadWord(bus, slot, func, 0x40));
        cprintf("pci 2nd IDETIM : %lx\n", pciConfigReadWord(bus, slot, func, 0x44));
        cprintf("pci BAR4       : %lx\n", pciConfigReadWord(bus, slot, func, 0x20));

    }
    return;
}

void
waitdisk(void)
{
    // 0x01F0-0x01F7  The primary ATA harddisk controller.
	// wait for disk reaady
	while ((inb(0x1F7) & 0xC0) != 0x40)
		/* do nothing */;
}


void
set_dma_read(uint32_t offset, uint8_t seccount)
{
	// wait for disk to be ready
	waitdisk();
    // cprintf("ready\n");
    outb(0x1F1, 1);  // for dma
	outb(0x1F2, seccount);
	outb(0x1F3, offset);
	outb(0x1F4, offset >> 8);
	outb(0x1F5, offset >> 16);
	outb(0x1F6, (offset >> 24) | 0xE0);
	outb(0x1F7, 0xc8);	// cmd 0xc8 - DMA read sectors, 28-bit LBA mode
}



void ide_bus_master_dma_test(void){
    uint32_t tmp;
    uint8_t command = BUS_MASTER_READ | BUS_MASTER_START;
    uint8_t status  = 0;
    cprintf("Testing Bus Mastering DMA\n");
    __asm __volatile("WBINVD");
    tmp = setBAR4(bus, slot, func, BUS_MASTER_START);
    if (BUS_MASTER_START != tmp) {
        cprintf("Warning: setting BAR4, 0x%lx is set but set as 0x%lx\n",
                BUS_MASTER_START, tmp);
    }
    

    // Prepare a PRDT in system memory.
    PRDBASE[0] = SET_PRDTABLE(0x200000, 512, 0);
    PRDBASE[1] = SET_PRDTABLE(0x200200, 512, PRDT_EOT);
    __asm __volatile("WBINVD");


    // Set the Start/Stop bit on the Bus Master Command Register.
    outb(BUS_MASTER_CMD,0x0); // command stop
    // Send the physical PRDT address to the Bus Master PRDT Register.
    outl(BUS_MASTER_PRDT, (uint32_t)__PRDBASE);


    // enable DMA on the device
    pciConfigWriteDWord(bus, slot, func, 4, 0x3A800005);
    pciConfigWriteDWord(bus, slot, func, 0x40, 0x80008000);

    // Clear the Error and Interrupt bit in the Bus Master Status Register.
    outb(BUS_MASTER_STAT, 0x6);
    // Select the drive. Send the LBA and sector count to their respective ports.
    set_dma_read(0, 2);

    // Send the DMA transfer command to the ATA controller.
    cprintf("Starting DMA\n");
    outb(BUS_MASTER_CMD, 1);
}