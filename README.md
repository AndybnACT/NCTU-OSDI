1. Why STA_R have same value as STA_W:
    -   Ans: see [OSDev](https://wiki.osdev.org/Global_Descriptor_Table). For data segment, read is always allowed. On the other hand, write is never allowed on code segments. Thus, we only need to decide whether write is granted on data and read on code. 
2. Remember that interpretation of 32-bit mode selector is not same as 16-bit mode:
    - In 16-bit real mode, if \$cs equals 0x0, then the program counter is decided directly by \$cs*16+$pc
    - In 32-bit protected mode, 0x0 points to the null entry of gdt with RPL=0 (lower two bits). And the address of program counter is indirectly determined by the entry of gdt + $pc.
3. Why getline interpret -1 as error, while getc return -1 to inform read complete?
---
## 7.Questions
#### GDT setup
`SEG(type,base,lim)`
1. Explain what are the parameters `type` `base` `lim` for, and how did you decide the value of them.
    - The macro defines a [global descriptor](https://wiki.osdev.org/Global_Descriptor_Table) in GDT. The meaning of parameters are listed below:
        - `type`:  
            - bit `0`: accessed bit, CPU marks as 1 when visiting the descriptor.
            - bit `1`: RW bit, determines whether read would be granted for code segments, and write for data segments.
            - bit `2`: Direction/Conforming bit. For code segments, this bit indicates if the segment can be executed from an equal or lower privilege level. For data segments, it tells the segment grows up(1) or down(0).
            - bit `3`: Executable bit, set to 1 for code segments; 0 for data segments.
        - `base`: 32-bit base address of the segment
        - `lim`: size of the segment in page (4KB) unit.
    - For kernel code segment: 
        - Set `STA_X|STA_R` for `type` of the segment. However, tests have shown that it is not necessary to set `STR_R` to code segments of nctuos.
        - To determine `base` of the macro, we need to look into the code, where CPU switches to 32-bit protected mode with kernel segment selector. This happens at `ljmp` instruction in `boot/boot.S`. To properly jump to `$protcseg` with kernel code segment selector, the sum of the offset, which is `$protcseg` at `ljmp`, and base address specified at the segment descriptor must be the physical address of `$protcseg` in memory. Since BIOS loads 1st sector, which is `boot/boot`, at the beginning of memory, the physical address of `$protcseg` is the value of `$protcseg` itself. Thus `base` of the descriptor must be zero.
        - I just set `lim` to a large number where there is little chance for program counter to go over it.
    - For kernel data segment:
        - The `STA_W` must set; otherwise kernel cannot properly boots.
        - As for `base` of the data segment, we may find the clue at `boot/main.c`. The program tries to load ELF to several addresses and start execution from the entry address of loaded ELF. Since all addresses of data and code go through `ds` (load/store) and `cs` (execute) respectively. And the entry point of ELF located in the same memory domain where it is loaded (offsets start from start of the ELF). The `base` of code must be identical to the `base` of data.
        - I just set `lim` to a large number where there is little chance for data to overflow.
---

 2. How did you setup gdtdesc, what's the minimum value of `gdt limit` which still boot properly?
 ```Clike
 lgdt gdtdesc
 .
 .
 .
 gdtdesc:
     .word # gdt limit <<<<
     .word # gdt base
 ```
  - `gdesc` will be loaded into GDTR when executing `lgdt gdesc`. GDTR is a 48 bits register on IA-32 architecture consisting an upper 32-bit linear base address and a lower 16-bit table limit in bytes of the GDT then to be loaded. We have 3 entries in GDT, an entry is 8 bytes long, so the first word (16 bits) ~~should be 24~~ . Following, the second and third words should be the physical(?) address of the GDT. Therefore, just fillup with `$gdt, 0x0`. we can replace `.word` with `.long`, if `$gdt` itself overflow a word. Then there is no need to fill up the leading `0x0`.  
  - minimum: 23. 23 does boot the system. However, the most significant byte of the GDT, that is the highest byte of `base` address in data descriptor, still take effect in upcoming operations. Tests have shown that the system wouldn't boot if we gave the highest byte of `base` address a non-zero value under this setting. After searching on the [Internet](https://en.wikibooks.org/wiki/X86_Assembly/Global_Descriptor_Table), we found that `limit` is 1 byte less than the length of the table. Thus, 24-1 = 23 determines length of a 3-entry table. Or we can use variables `gdtdesc-gdt-1` to automatically adjust the limit.
  - [OSDev](https://wiki.osdev.org/Global_Descriptor_Table): The size is the size of the table subtracted by 1. This is because the maximum value of size is 65535, while the GDT can be up to 65536 bytes (a maximum of 8192 entries). Further no GDT can have a size of 0.
 ---
 3. What do the below instructions do in boot/boot.S.
 
 ```Clike
movl    %cr0, %eax
orl     $CR0_PE_ON, %eax
movl    %eax, %cr0
```
 - it turns protected mode on without changing the rest of part in `cr0` register.
### IDT setup
`SETGATE(gate, istrap, sel, off, dpl)`

1. What are the 1st and 4th parameters of `SETGATE` for?
    1. `gate`: It is a `struct Gatedesc` object. All properties of a gate specified on the macro will be handled and loaded into proper locations of the object. 
    2. `dpl`: Descriptor Privilege Level. All interrupts check if value of current privilege level (CPL) is less than DPL for privilege protection. When a hardware interrupt occurs, before switching to the ISR, it sets CPL as 0 to pass the test [[ref]](https://books.google.com.tw/books?id=pbB8Z1ewwEgC&pg=PA162&lpg=PA162&dq=hardware+interrupt+cpl&source=bl&ots=pt_w1Bdvdg&sig=ACfU3U27QPUnwTZu1GMCWaexxyMN15bToA&hl=zh-TW&sa=X&ved=2ahUKEwjc6vCi_J7hAhXwx4sBHR6IA6QQ6AEwAXoECAUQAQ#v=onepage&q=hardware%20interrupt%20cpl&f=false). In this way, setting `dpl` as 0 allows ISRs to be protected from user programs to activate hardware routines by just issuing `int` with certain service numbers.
    3. `off`: offset (logical address) of the ISR. 
2. After `lidt`, is data structure `Pseudodesc` still used for other purpose, or we can just discard it？Why?
    - We can just discard it, since the content of `Pseudodesc` will be loaded into a special 32+16 bits register, IDTR.

### ELF kernel image loading
`readseg((uint32_t) ELFHDR, SECTSIZE*8, 0);`
1. What the above code is for? It loads data from which location of disk to what address of memory?
    - The code utilize x86 [I/O_port](https://wiki.osdev.org/I/O_Ports) [instructions](https://c9x.me/x86/html/file_module_x86_id_139.html) to load data from first ATA disk into memory. The destination address of the data in memory is decided by `ELFHDR` and rounded down to sector boundary (512B). Similarly, the source address of data coming from the disk is determined by offset and round up to sector boundary. The purpose of this line is to load kernel program headers into memory. Then, based on the information at each program header, the loader is able to load kernel into correct addresses. However, it is worth mentioning that according to `$readelf -h kernel/system`, it seems that it is not necessary to load a 4KB page into memory in order to get all program headers since there are only two program headers and each of it is 32 bytes long, starting from 52 bytes offset. And tests had revealed that using merely `SECTSIZE` still boot the kernel.
    ```
    [osdi@localhost nctuos]$ readelf  -h kernel/system
    ELF Header:
    Magic:   7f 45 4c 46 01 01 01 00 00 00 00 00 00 00 00 00 
    Class:                             ELF32
    Data:                              2's complement, little endian
    Version:                           1 (current)
    OS/ABI:                            UNIX - System V
    ABI Version:                       0
    Type:                              EXEC (Executable file)
    Machine:                           Intel 80386
    Version:                           0x1
    Entry point address:               0x100000
    Start of program headers:          52 (bytes into file)<--------------
    Start of section headers:          42072 (bytes into file)
    Flags:                             0x0
    Size of this header:               52 (bytes)
    Size of program headers:           32 (bytes) <-----------------------
    Number of program headers:         2          <-----------------------
    Size of section headers:           40 (bytes)
    Number of section headers:         21
    Section header string table index: 18
    ```
    - The function `readseg` first rounds addresses into a proper boundary. Then it calls `readsect` to read a sector once, until data reaches count.  

3. Which file decides the kernel elf image's e_entry value? 
    - The linker script `kernel/kern.ld` defines the entry point of the kernel with the symbol `kernel_load_addr`, which is `0x100000`.

### Keyboard and Screen
1. Describe the entire procedure from a keystroke to the console print the character. You should show TA the code rather than just oral speaking.
    1. if keyboard interrupt is correctly set, a keypress informs CPU with 0x21 interrupt. An interrupt causes CPU stores some registers on handler stack, then invoke handler in `trap_entry.S`, specifically `kbd_trap_handler`, based on the interrupt number. The handler here just store some additional registers value and interrupt number on the stack, where the following `C` function `default_trap_handler` uses as a local variable `struct Trapframe *tf`.
    ![](https://i.imgur.com/pxydT3U.png)
    2. Diving into `default_trap_handler`, it calls `trap_dispatch`, which then calls the actual service routine based on `tf->tf_trapno`. In our case, it calls `kbd_intr`.
    3. In the service routine of keyboard interrupt, `cons_intr(kbd_proc_data);`, a while loop calls `kbd_proc_data` to gets a character from device through IO port, and store the character on a queue (buffer), `cons`, if it get a valid character. The queue may discard previous keypresses if the buffer is full (`CONSBUFSIZE`) but valid characters still keep coming from `kbd_proc_data`. 
        - The function `kbd_proc_data` lookups several table to map keystrokes to the final character.
        - The function used to consume valid key presses is `cons_getc` called by `getc`. 

2. Uncomment kbd.c:199 `kbd_intr()` and comment out your keyboard handler function call. Everything works as if the same. What's the difference of the original one's mechanism and modified one's mechanism?
    - In addition to commenting out keyboard handler, we need to comment out `kbd_init();` in `kernel_main()` function to fully disable keyboard interrupt. Otherwise, any keypress still causes CPU to lookup the IDT entry specified by `kdb_init()`.
    - polling