### nctuOS

A tiny OS that used for course OSDI in National Chiao Tung University, Computer Science Dept.

This OS only supports x86

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


### Lab 5

In this lab, you will learn about process management, basic scheduling and system calls.

In our design, each process contains a page directory so that each of them can have full memory addressing space.

For each process, there are several attributes that are necessary for kernel to manage the process easily, you can reference `kernel/task.h` for more detail.

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

**Modifications to be made**
- `lib/syscall.c` implement the interfaces of system calls for user program to use.
- `kernel/task.h` to open interfaces of implementation of system calls for `kernel/syscall.c`
- `kernel/task.c` implement `task_create`, `task_free`, `sys_kill`, `sys_fork`, and `task_init`
- `kernel/syscall.c` implement system calls
- `kernel/trap_entry.S` implement trap handler interface for system call
- `kernel/mem.c` implement `setupkvm` used by each process creation
- `kernel/sched.c` implement scheduler
- `kernel/timer.c` implement timer_handler to support sleep

## Questions
## 1. How did you implement "your" super perfect elegant scheduler?
## 2. systemcall & interrupt
1. How do we pass the arguments and the return value when doing system call？
    There are at most 5 arguments available for system calls on`nctuos`, which are `$edx`, `$ecx`, `$ebx`, `$edi`, and `$esi` respectively. `$eax` is used to specify the system call number. After the system call returns, `$eax` is again used to place the return value.
2. How is stack set when interrupt occurs in user mode？ How about kernel mode？
    If interrupt occurs in user mode and the interrupt handler (code selector) is located in kernel mode, the switching of privilege level alters the stack pointer as well. `$esp` would change according to `esp0` field in `tss`. The original value of `$ss`, along with `$esp`, `$EFLAGS`, `$cs`, `$eip`, etc... would then be stored on the new stack respectively. On the other hand, if the interrupt happens in kernel mode and the handler uses the code segment that has the same privilege level, the CPU just pushes registers starting from `$EFLAGS` on the same stack.
    ![](https://i.imgur.com/pxydT3U.png)
3. Could you implement a non-shared kernel stack in the nctuOS？Could you do it with only one tss?
    - By default, kernel stack is shared across different processes. The kernel stack is physically located by `kernel/entry.S` and virtually mapped at `bootstack+KSTKSIZE` using `boot_map_region` in `mem_init`. When a new task is created, code, data, and stack are also mapped using `boot_map_region` in the same manner. Thus, every switch from users to the kernel makes `$esp` switch to the same virtual, and therefore, same physical address.  
    - It is possible to keep a private kernel stack for each process. Just allocate stack pages, copy current stack content to the new stack when creating a new task using functions provided by `kernel/mem.c`. And free the stack pages before freeing the task. Please refer to commit `9183e58` for implementation details.

## 3. process
1. Based on NCTU-OS, which of the following items are shared between our forked tasks?
    - [ ] user stack
    - [x] user data
    - [x] user code
    - [x] kernel stack (configurable)
    - [x] kernel data
    - [x] kernel code
2. How do we save and restore the contents of registers and stack when context switch?
    - Every context switch takes place in kernel mode. Therefore, content of all general registers must have been preserved on stack through `pushal` when entering `_alltraps` in `kernel/trap_entry.S`. Then `trap_dispatch` must have copied it into per-process's trap frame, in which can be globally referenced by `tasks[pid].tf.tf_regs`. As for program counter, stack pointer, and eflag register, they were pushed to the kernel stack automatically when entering kernel mode, then copied to `tasks[pid].tf` in the same way. 
    - To restore the execution of a previously stopped task, we have to load the process context into corresponding registers. As mentioned above, we can access the selected process's user-mode context through `tasks[pid].tf`. Then, with the help of `env_pop_tf`, it restores general registers by setting `$esp` to the trap frame itself, and popping along it. Then `iret` loads the `$cs`, `$eip`, `$ss`, `$esp` from it by hardware. After everything is done, the process can continue its user-mode execution from where it was stopped.

[iret](https://www.felixcloutier.com/x86/iret:iretd)
[reference](https://compas.cs.stonybrook.edu/~nhonarmand/courses/sp17/cse506/slides/04-interrupts.pdf)
## 4. Gate
1. When setting user code and data segment in gdt(kernel/task.c), why we set the dpl value to be 3?
    `dpl` specifies the minimum required privilege level that a calling `cpl` must have when entering that segment using software interrupt, `int`. 
2. How do you determine the dpl value of every interrupts and trap gates? (ex: system call, timer, keyboard, page fault) Please refer to Chap 6.12 of the Intel manual Vol3.
    Only system calls should be able to be entered using software interrupt at lower privilege level.  
