## 6. Question
1. Please explain the following functions:
* boot_alloc
    - `boot_alloc` uses the symbol, `end`, defined at the linker script `kern.ld`, as the first starting address of allocations. It means that `boot_alloc` allocates memories starting from the end of kernel image. Starting from `end`, every allocation follows the last one and is rounded up to a 4KB boundary.
* page_init
    - After `pages`, a 1-to-1 mapping array that describes allocation status of all physical pages, is allocated and zero-initialized, `page_init` sets entries of the array according to current layout of the physical memory. It marks the first page, IO-hole pages, and pages currently occupied by the kernel as used, sets the rest of pages as free, and chains it together as a linked-list `page_free_list`.  
* page_alloc
    - This function takes a page descriptor from `page_free_list` and returns it to the caller. the page is zero-initialized if required. Once a page is allocated, it will be taken out of `page_free_list` and never be allocated again until it is freed.  
    - I don't know why it is callers' responsibility to increase the reference count of the acquired page (`PageInfo`).
* page_free
    - If reference count of a page reaches zero, the page should be freed and returned to `page_free_list`. It is `page_free` in response to perform this action. 

2. Please explain the following functions:
* pgdir_walk
    - Given a virtual address `va` and the belonging page directory `pgdir`, `pgdir_walk` returns the virtual address of PTE that holds `va`. if the page table does not exist, that is, the PDE gives a NULL address, then `pgdir_walk` creates the table, setups the belonging PDE, and returns the address under `create` mode. Otherwise, `pgdir_walk` returns a NULL pointer if `create==0`, or if it fails to allocate the page table.    
    ![](https://i.imgur.com/EakveZL.png)
    - How to set permission of PDE correctly?
* page_lookup
    - `pgdir_walk` returns a pointer to the PTE representing the page that an input address `va` resides. On the other hand, `page_lookup` returns a pointer to the page descriptor that maps to the physical page of `va`.
* boot_map_region
    - The function maps pages of virtual address starting from `va` with `size` to physical address `pa` in a continuous manner. It gets addresses of PTEs by `pgdir_walk` under `create` mode, sets physical addresses and permissions correspondingly, and move on to the next page.
* page_remove
    - Remove a page that the input `va` belongs to from its page table, and decrease reference count of that page by `page_decref`.
    - How do we reclaim a page table page if it is no longer used?
* page_insert
    - Provided by a page descriptor `pp` just allocated, `page_insert` inserts the page into the page table structure so that further accesses to `va` reflect on that page. 

3. Please show how the physical address space and the virtual address space change from booting to the end of mem_init.
    1. From the very first stage of booting, when BIOS loads the first sector of disk image `kernel.img` into memory at physical address `0x7c00`, CPU are in 16-bit real mode. Thus, virtual addresses are mapped to physical addresses by `(physical addr)[0:20] = $(segment selector)[0:15]*16 + (virtual address)[0:15]`. We can find physical memory layout at this stage from [OSDev](https://wiki.osdev.org/Memory_Map_(x86)#Extended_BIOS_Data_Area_.28EBDA.29). Some relevant components are listed below:
    
    | Physical Range | Size     | Description (type)|
    | :------------- | :------- | :----------------------------------|
    | 0x0-0x3FF      |  1KB     | BIOS Interrupt Vector Table (RAM)  |
    | 0x7C00-0x7DFF  | 512 Byte | Second stage boot loader (RAM)     |
    | 0xA0000-0xFFFFF| 384KB    | VGA, MMIO, BIOS code (various)     |
    
    2. After the second-stage boot loader is loaded and invoked at `0x7c00`, it setups `gdt` and jumps to 32-bit protected mode (see `boot/boot.S`). From now on, virtual addresses go through [segmentation](https://wiki.osdev.org/Segmentation#Protected_Mode) to translate into physical addresses. However, virtual addresses are equivalent to physical addresses in our case since all of our segment descriptors have `0x0` base address (see `gdt` at `boot/boot.S`). Then, we call `bootmain` at `boot/main.c`.
    3. `bootmain` is an ELF loader. It loads ELF header and program headers at `0x10000`. Based on informations at program headers, it loads program with size `p_filesz` rounded up to a sector boundary at address `p_pa`. In our case, the kernel is loaded at `0x100000`, then it jumps to the entry point of the kernel.
    ```//C
    for (; ph < eph; ph++)
        // p_pa is the load address of this segment (as well
        // as the physical address)
        readseg(ph->p_pa, ph->p_filesz, ph->p_offset);

    // call the entry point from the ELF header
    // note: does not return!
    ((void (*)(void)) (ELFHDR->e_entry))();
    ```
    
    | Virtual Range| Physical Range | Size     | Description (type)|
    |:-------------|:------------- | :------- | :----------------------------------|
    |0x0-0x3FF      | 0x0-0x3FF      |  1KB     | BIOS Interrupt Vector Table (RAM)  |
    |0x7C00-0x7DFF  | 0x7C00-0x7DFF  | 512 Byte | Second stage boot loader (RAM)     |
    |0xA0000-0xFFFFF| 0xA0000-0xFFFFF| 384KB    | VGA, MMIO, BIOS code (various)     |
    |0x10000-0x11000|0x10000-0x11000| 4KB       | First 4KB of our kernel ELF image  |
    |0x100000-0x108000|0x100000-0x108000|32KB   | Kenel code + data |
    4. The entry point of kernel code is defined at `kernel/entry.S`, which turns on paging unit with two predefined page tables (`kernel/entry_pgdir.c`) that maps linear address `0-4MB` and `KERNBASE-KERNBASE+4MB` to physical address `0-4MB`. Although the `reloacated` routine does reload `gdt`, contents of new `gdt` are same as the previous one. Thus, only paging unit is effectively in used for address space translation. Note that when using symbols here before paging, we need to manually convert its addresses by subtracting `KERNBASE=0xF0000000`. The reason is that though `kern.ld` linker script has defined the starting address of the kernel is `kernel_load_addr=0xF0100000`, they are actually loaded at `0x100000`.
    ```C
    __attribute__((__aligned__(PGSIZE)))
    pde_t entry_pgdir[NPDENTRIES] = {
	// Map VA's [0, 4MB) to PA's [0, 4MB)
	   [0]
		    = ((uintptr_t)entry_pgtable - KERNBASE) + PTE_P + PTE_W,
	// Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
	   [KERNBASE>>PDXSHIFT]
		    = ((uintptr_t)entry_pgtable - KERNBASE) + PTE_P + PTE_W 
    };
    __attribute__((__aligned__(PGSIZE)))
    pte_t entry_pgtable[NPTENTRIES] = {...}
    ```
    - After paging is enabled, memory would look like below. We need to have low memory map because program counter still remains at the low area before jumping to `$relocated`
    
    | Virtual Range| Physical Range | Size     | Description|
    |:-------------|:------------- | :------- | :----------------------------------|
    |0x00000000-0x00040000   | 0x0-0x400000  | 4MB      | BIOS, kernel, ... (see section 3.) |
    |0xF0000000-0xF0400000   | 0x0-0x400000  | 4MB      | BIOS, kernel, ... (see section 3.)|
    5. Then, with this basic paging structure, `entry.S` calls into `kernel_main` and the kernel starts initialize itself. It setups interrupt controller, performs startup routines for keyboard and timer, and setup IDT. All of these actions are perform within virtual range of `0xF0000000-0xF0400000`. The kernel here does not dynamically allocate memory, instead, data blocks are already defined on `.bss` and `.rodata`. 
    6. Entering `mem_init`, the kernel first detects how many memory it owns with `i386_detect_memory`. Then, it allocates a page directory `kern_pgdir` for future replacement of current page directory, and an 1-to-1 mapping array which maps one allocation status (`struct PageInfo`) to each physical 4KB page constitutively. `boot_alloc`'s allocations start at the end of `.bss` and will not incur errors as long as upper bound of data, `nextfree`, does not go out of `0xF0400000`.
    7. After `page_init` settles down `struct PageInfo *pages`, `boot_map_region` tries to install paging structure on `kern_pgdir` of kernel itself. if everything works fine, the kernel loads physical address of `kern_pgdir` onto the control register using `lcr3()`. Once `$cr3` points to the newly generated page directory, kernel has paging ready, and the virtual to physical memory layout would look like the following:
    
    | Virtual Range                | Physical Range       |Description|
    | :--------------------------- | :------------------- |:----------------------------------------------|
    | UPAGE:some 4K boundary       | PADDR(pages)         |   Read-only copies of `struct PageInfo *pages` for user, possibly leaking??|
    | KSTACKTOP-KSTKSIZE:KSTACKTOP | PADDR(bootstack)     | Kernel stack|
    | 0xF0000000-0xFFFFFFFF        | 0x0-0xFFFFFFF        | Kernel image, BIOS routine, MMIO, parts of user mem??|
    | IOPHYSMEM-EXTPHYSMEM         | IOPHYSMEM-EXTPHYSMEM | MMIO, only used by kernel|
4. Why do we need RELOC at line 8 and line 24 in kernel/entry.S?
> \_start = RELOC(entry)
> movl $(RELOC(entry_pgdir)), %eax
- As explained above, kernel is loaded at physical address `0x100000` while addresses of symbols start from `0xF0100000`. At this time, we have not enabled paging unit to translate symbol addresses to where it is actually located. Therefore, manual translation is needed before paging.


5. why we use`mov $relocated, %eax; jmp *eax`instead of`jmp relocated`? If we use the later one, will it incur page fault? If yes, where the page fault incurs. If no, explain the reason.
    - We can observe differences by `objdump -d` the kernel ELF file. If we use the former one, the `jmp` instruction has `eb` opcode. While the later one generates `ff` opcode. According to [x86 instruction set reference](https://c9x.me/x86/html/file_module_x86_id_147.html), `jmp` with `eb` opcode is a near jump, which adds program counter by a signed 8-bit integer taken from the following byte. Therefore, that `jmp` instruction is impossible to set EIP to high memory, `KERNBASE`. In fact it just take the physical address of `relocated` as virtual one and jumps there. Everything works normally since `entry_pgdir.c` maps low memory as well. However, since the following calls and jumps all take relative addresses, `EIP` will still run at low address when loading new paging structure, where there is no mapping from low virtual memory to low physical one, at `mem_init`. Consequently, the first dereference of the program counter after loading `cr3` incurs page translation fail, resulting the page fault.
    
    >>>>> ![](https://i.imgur.com/YZUoNwe.png)
```
f0100f5c <mem_init>:
...
f0101f9b:       0f 22 d8                mov    %eax,%cr3   # install new paging structure
f0101f9e:       31 c0                   xor    %eax,%eax   #<---physically loaded at 0x101f9e
...
```

6. After paging, how does the mmio mechanism change?
    - After kernel sets `kern_pgdir` as base of page directory, mmio range is accessible by giving virtual address `IOPHYSMEM` or `KERNBASE+IOPHYSMEM`.
7. My question: What does `movw	$0x1234,0x472			# warm boot` at `kernel/entry.S` mean?
