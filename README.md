## Questions
1. What's the meaning of `ljmp $BOOTSEG, $_start`(boot/bootsect.s)? What is the memory address of the beginning of bootsect.s? What is the value of `$_start`? From above questions, could you please clearly explain how do you jump to the beginning of hello image?
> `ljmp $BOOTSEG, $_start` will load `$BOOTSEG=0x7c00` to `$cs` register and jump tp offset `$_start=5` (use `gdb` or  `objdump` the linked file to checkout)
> ```
(gdb) set architecture i8086
(gdb) b *0x7c00
(gdb) c
...
(gdb) x/10i $pc + $cs*16
=> 0x7c00:	ljmp   $0x7c0,$0x5 #--->$_start
   0x7c05:	mov    $0x7c0,%ax
   0x7c08:	mov    %ax,%ds
   0x7c0a:	mov    $0x9000,%ax
   0x7c0d:	mov    %ax,%es
   0x7c0f:	mov    $0x100,%cx
   0x7c12:	sub    %si,%si
   0x7c14:	sub    %di,%di
   0x7c16:	rep movsw %ds:(%si),%es:(%di)
._._._._._._._._._._._._._._._._._._._._._._._._.
$ ld -m elf_i386 -Ttext 0 -o bootsect bootsect.o 
$ objdump -d -mi8086 bootsect
Disassembly of section .text:
00000000 <begtext>:
   0:	ea 05 00 c0 07       	ljmp   $0x7c0,$0x5
00000005 <_start>:
   5:	b8 c0 07             	mov    $0x7c0,%ax
   8:	8e d8                	mov    %ax,%ds
   a:	b8 00 90             	mov    $0x9000,%ax
   d:	8e c0                	mov    %ax,%es
```
> At the beginning of `bootsect`, specifically `$_start` block, each time  when `rep` is hit, CPU copies 2 bytes of data starting from `%ds:%si` to `%es:%di`, increases `%si` `%di`, and decreases `%cx` by 1. The operation keep copying data from `0x7c0:%si` to `0x9000:%di` until `%cx` reaches zero. By then, all 512 bytes of `bootsect` will be copied to `0x90000` and `ljmp` continues the execution there.
> ```
_start:
	mov	$BOOTSEG, %ax
	mov	%ax, %ds
	mov	$INITSEG, %ax
	mov	%ax, %es
	mov	$256, %cx
	sub	%si, %si
	sub	%di, %di
	rep	
	movsw
	ljmp	$INITSEG, $go
    ```
> Then `go` setups the rest of segment registers. The code segment register has been switched to `0x9000` since last instruction.
    ```
go:	mov	%cs, %ax
	mov	%ax, %ds
    mov	%ax, %es
;#  put stack at 0x9ff00.
    mov	%ax, %ss
    mov	$0xFF00, %sp		# arbitrary value >>512
```
> To load the hello program into memory and start executing, we need to invoke 0x13 BIOS interrupt call. According to BIOS specification, we should provide several arguments for `int $0x13`. `%ah=2` tells disk to perform 'read'. For 'read' service, `%al` specifies number of sector being read, `dx` `cx` locates the data in disk, and `%es:%ax` is the address of the read buffer.
> ```
load_hello:
;# Load hello at 0x10000    
    mov $0x0000, %dx      # drive 0, head 0
    mov $0x0002, %cx      # sector 2, track 0 >>> what's track??
    xor %bx, %bx
    mov $0x0100, %ax
    mov %ax, %es          # set buffer ptr = 0x10000
    .equ AX, 0x0200+HELLOLEN
    mov $AX, %ax          # service 2, nr of sector=1
    int $0x13
    jnc ok_load_hello     # jump if (CF == 0)
    xor %dx, %dx
    xor %ax, %ax
    int $0x13             # reset the disk
    jmp load_hello
```
> In our implementation, `load_hello` tries to load sector 2 of disk 0 (floppy) to address `0x1000`. If there's no error report from disk (`CF`), then `ok_load_hello` will load `0x100` to code segment register and jump to `hello`. Otherwise it will reset the disk and try again. 
>```
ok_load_hello:
    .equ sel_cs0, 0x0100 #select for code segment 0
    ljmp $sel_cs0, $0 #Jump to hello
    ;# cpu will not return here
```
2. What's the purpose of es register when the cpu is performing int $0x13 with AH=0x2h? 
> There are a number of disk [services](http://stanislavs.org/helppc/int_13.html) in `int $0x13` such as reset, read, and write, provided by BIOS. `%ah` is the parameter indicating what disk service is going to perform. `%ah=2` informs BIOS to read from a disk. 

3. Please change the Hello program's font color to another
> Hint: INT10H  
> Just set `%bl` to some other [values](https://en.wikipedia.org/wiki/BIOS_color_attributes).

4. If we would like to swap the position of hello and setup in the Image. Where do we need to modify in tools/build.sh and bootsect.s? 
> boot/bootsect.s:
    - sector being read @ `load_hello` and `load_setup` (`%cx`)  
> 
> toos/build.sh:
    - seek number of `hello` and `setup`
    - Note: those who with a lower seek should do `dd` first (I don't know why)

5. Please trace the SeaBIOS code. What are the first and the last instruction of the SeaBIOS? Where are they?
> The first instruction that seabios executes is `ljmpw`, which loads the following two immediates as code segment `$cs` and a 16 bits(word) offset `$ip`, then jumps there:  
>> Using `readelf -h out/rom.o`, we can find the entry point of the program is `0xffff0`. Then we may find the corresponding symbol and therefore search it in the source file by `nm out/rom.o | grep ffff0`. Luckily, the command tells us that `reset_vector` is symbol of the entry point. As such, we can find the first instruction at `src/romlayout.S` showing:
> ```
reset_vector:
        ljmpw $SEG_BIOS, $entry_post    
 ```
>
> The last instruction that seabios executes is `iret`. In real mode, the instruction pops two values stored on top of the stack as `$ip` and `$cs` then begins execution there.
![Imgur](https://i.imgur.com/fme9MUs.png)
>> The last C function invoked in Seabios during our startup is `call16` called by `stack_hop_back`, `farcall16`,  `call_boot_entry` and `boot_disk`. `call16` is a wrapper function making subsequent calls transfer CPU to 16 bits mode (`transition16`) then jump to a correct code segment and register state (see `iret` at `__farcall16`) provided by a `struct bregs *` that has been casted to `u32` at `stack_hop_back`. In our case, the call to `call_boot_entry` defines the target address of `struct bregs *`.
>```
0xfcca6 <_rodata32seg+25142>:	push   %ebp #Starting address of __farcall16 
...
0xfccb6 <_rodata32seg+25158>:	pushl  0x20(%eax)
...
0xfcce7 <_rodata32seg+25207>:	iret 
```
>   
> Note: Native `gdb` cannot properly handle real mode addressing. Addresses of program counter and breakpoints used by `gdb` does not take `$cs` into consideration. As the result, when setting break points at 16-bit mode, one should subtract absolute address of the breakpoint with corresponding `$cs` value. On the other hand, to show current executing real mode instructions, please do:
> ```
(gdb) set architecture i8086 
(gdb) x/[N]i $pc + $cs*16 # replace '[N]' with number of instructions
```
> Or, refer to the link [here](http://ternet.fr/gdb_real_mode.html) to save your days.
> 
> Hint 1: You may want to use debugger to find the first instruction. Run qemu with -S can pause the cpu at the beginning. Remember? In addition to that, `readelf` is a convenient tool to read executable file's symbol table.
> [SeaBios Debugging](https://www.seabios.org/Debugging)  
> 
> Hint 2: SeaBios will try to find the MBR magic number 0xaa55. If there is a 512 bytes sector which ends with 0xaa55, that sector is the boot sector.
> Last instruction: The instruction before SeaBios give the cpu to OS.
