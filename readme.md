### nctuOS

A tiny OS that used for course OSDI in National Chiao Tung University, Computer Science Dept.

This OS only supports x86
### Lab 7

In this lab, you will learn about how file system implement in an OS.

You can leverage `grep` to find out where to fill up to finish this lab.

`$ grep -R "TODO: Lab7"`

To run this kernel

    $ make
    $ make qemu

To debug

    $ make debug

### Lab 6

In this lab, you will learn about how to make os support symmertric multiprocessing (SMP) and simple scheduling policy for SMP.

**Source Tree**

`kernel/*`: Includes all the file implementation needed by kernel only.
`lib/*`: Includes libraries that should be in user space.
`inc/*`: Header files for user.
`user/*`: Files for user program.
`boot/*`: Files for booting the kernel.

You can leverage `grep` to find out where to fill up to finish this lab.

`$ grep -R TODO .`

To run this kernel

    $ make
    $ make qemu

To debug

    $ make debug

## Questions
## 6.1 mp_init():
* What does mp_config do?
    `mp_config` searches some area in EBDA (Extended BIOS Data Area) for a byte signature "\_MP_", which represents the MP Floating Pointer Structure. Then it returns the address of corresponding configuration table, where we use for finding processor topology.
* What does this loop do?
    The following loop uses the address given by `mp_config` to identify number of processors. According to spec, `conf->entry` should indicate number of entries in this table. Size of an entry should be 8 bytes if it is not a processor, or 20 bytes otherwise. Therefore, the index `i` lets the program loop over the entire table while `p` helps locating the entry of different types. 
```clike
for (p = conf->entries, i = 0; i < conf->entry; i++) {
    ...
}
```

* What is the interrupt mode for the nctuOS?
    According to the MP specification, there are three possible interrupt modes, which are PIC mode, virtual wired mode, and symmetric I/O mode. ncutOS is in virtual-wired mode.

## 6.2 How did you modify:
* task_create()
* sys_fork()
* sys_kill()
 
## 6.3 How did you modify the scheduler for the SMP system?

## 6.4 What does boot_aps do?
* How does the mpentry_kstack setup?
    `mpentry_kstack` is the stack pointer of 
* What does lapic_startup do(mechanism)?
* What's AP's initial program counter after it wake up?
