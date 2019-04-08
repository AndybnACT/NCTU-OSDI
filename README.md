Num     Type           Disp Enb Address    What
1       breakpoint     keep n   0x00008931 in find_empty_process at fork.c:141
	breakpoint already hit 4 times
2       breakpoint     keep n   0x00008422 in copy_process at fork.c:74
	breakpoint already hit 3 times
3       breakpoint     keep y   0x00009b06 in tell_father at exit.c:84
	breakpoint already hit 8 times
4       breakpoint     keep n   0x00008798 in copy_process at fork.c:134
	breakpoint already hit 3 times
5       breakpoint     keep y   0x00006984 in init at init/main.c:175

## Q1: QEMU
>> ```qemu-system-i386 -m 16M -boot a -fda Image -hda ../osdi.img -s -S -serial stdio```  
>
> According to the above command:  
Q1.1: What’s the difference between -S and -s in the command?  
> Ans1.1:  
> - According to the man page of qemu, -S flag will not start CPU at startup, which means the machine will pause at startup. On the other hand, -s will start a gdbserver listening on port 1234, which then can be used for remote debugging. 
>  
> Q1.2: What are -boot, -fda and -hda used for? If I want to boot with ../osdi.img(supposed it’s a bootable image) what should I do?  
>  Ans1.2:
>   - -boot: specifies the boot order. For example, on x86 machine, use a, b (floppy); c (1st HD); d (1st CD) to boot the target machine.
>   - -fda: uses file as floppy disk image (letter 'a' means it is the first image)
>   - -hda: uses file as hard disk image 
>   - Since `../osdi.img` is a disk image, if the image was considered to boot the target (x86) machine, option `c` should be passed together with `-boot` flag.
>>    `qemu-system-i386 -m 16M -boot c -fda Image -hda ../osdi.img`
## Q2 Git:
> Q2.1: Please explain all the flags and options used in below command:
>> ```git checkout -b lab1 origin/lab1```  
>
> Ans2.1: 
>   - `git checkout -b`: creates a new branch 'lab1' and switches to the branch locally.
>   - `origin`: reference to the remote repository
>   - `lab1`: branch name
>   - the command will checkout branch `lab1` from remote `origin` to local new branch name `origin`. ([link](https://stackoverflow.com/questions/1783405/how-do-i-check-out-a-remote-git-branch))
>
> Q2.2 What are the differences among git add, git commit, and git push? What’s the timing you will use them?  
> Ans2.2:
>   - `git add`: stages files -->1
>   - `git commit`: commits staged files -->2
>   - `git push`: makes changes take place on remote site -->3
## Q3 Makefile:
> Q3.1: What happened when you run the below command? Please explain it according to the Makefile.
>> ```make clean && make```  
>
> Ans3.1:
> - The script will delete all object files on top-level directory and call make clean in each subdirectory. 
>
> Q3.2: I did edit the include/linux/sched.h file and run make command successfully but the Image file remains the same. However, if I edit the init/main.c file and run make command. My Image will be recompile. What’s the difference between these two operations?  
> Ans3.2:  
>   - Ideally, if `sched.h` get modified, all rules depending on the file should be evoked. For example, there is a rule stating that `memory.o` have to be recompiled if there is an updated `sched.h`. However, it is obvious that typing `make` at the top-level directory never enters `mm/` and perform such job, since `mm/mm.o` depends on nothing at the top-level Makefile. As such, the parts that should have been recompiled and merged, resulting a different `Image`, are missing. 
>       - solution:
>```
                mm/mm.o: force
                    make -C mm
                force:
                    true
                ```
>   - Although the top-level Makefile specifies that `init/main.o` should depend on the `sched.h` and the recompilation indeed occurs, the resulting `main.o` and `Image` still make no difference. The reason is that `main.c` actually only uses one reference, `sched_init`, coming from `sched.h`. Since global references are resolved at link time depending on where it is actually declared, there is no way to affect `main.o` from `sched.h`. Besides, `main.c` does not declare any variable defined from `sched.h` so changes of variable definitions do not alter `main.o` either.
> 
>   ```
    [osdi@localhost init]$ nm main.o | grep sched
                  U sched_init
    [osdi@localhost linux-0.11]$ nm tools/system | grep sched
         000076da t reschedule
         00007502 T sched_init
```
> 
>    - solution: Build the image with original 0.11 kernel and Makefile, define `sched_init` as bellow in `sched.h`, and directly invoke `make` at the top-level directory. Note that if you `make clean && make`, you would get redefinition error at compile time when compiling `sched.c`. Reasons are stated in the previous section.
> ```  
        static inline void sched_init(void){
            printk("hacked");
        }// good article https://code.i-harness.com/en/q/3c5fd6 
        ```  
> To see exactly what `make` is doing, remove '@' characters at every command: `find . -name 'Makefile' |xargs sed -i -e "s/(control-v-tab)@/(control-v-tab)/"`
## Q4 After making, what does the kernel Image ‘Image’ look like?
> Ans4: It's a bootable floppy disk image. Looking into the file `build.sh`, we can find that `dd` tries to copy parts into different sectors of `Image`.
>   - `of=`: source file
>   - `if=`: destination file
>   - `bs=`: sector size in byte
>   - `count=`: number of sector being written
>   - `seek=`: starting sector of the destination file