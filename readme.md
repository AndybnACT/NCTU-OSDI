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

## 6. Question

## 1. How did you implement `open`, `close`, `read`, and `write`?
* Open
* Close
* Read
* Write
## 2. How did you implement `ls`, `touch`, and`rm`？
## 3. fs_init
* Are file descriptors in nctuos shared by all process or not?
* Describe the flow of mounting a filesystem in nctuos

## 4. What's the maximum number of files with size 1 byte?
**assume that the disk's size is 32MB, and using ```f_mkfs("/", 0, 0)``` .**
**you can just give an approximation with error < 100**
* How do you get this number?
* How to enlarge the maximum number of files without changing hardware?
* What the disadvantage of of such a modification in previous question ?

**\* Hint:** it's related to au(allocation unit size)
