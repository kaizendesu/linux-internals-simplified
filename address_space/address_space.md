# Linux Kernel Internals Simplified: Virtual Address Spaces

First, let's start with the simple program from the book.

```c
#include <stdio.h>
#include <stdlib.h>

static int static_var;
int global_var;

int		main(void)
{
	int		stack_var = 0;
	int		*heap_var;

	heap_var = malloc(sizeof(int));
	printf("static_var: %p\nglobal_var: %p\n", &static_var, &global_var);
	printf("stack_var: %p\nheap_var: %p\n", &stack_var, heap_var);
	free(heap_var);
	return 0;
}
```

This program prints the address of every kind of variable (global, static, stack, and heap)
to give us an idea of where the _data_ segment, _stack_ segment, and the _heap_ segments
are located in virtual address space. By segment, I mean the traditional Unix segment.

```txt
# gcc main.c && ./a.out
static_var: 0x404038
global_var: 0x40403c
stack_var: 0x7ffc8601ecc4
heap_var: 0x1e6d2a0
```

If we pad these values manually so that they are complete 64-bit addresses we get:

```txt
static_var: 0x0000000000404038
global_var: 0x000000000040403c
stack_var:  0x00007ffc8601ecc4
heap_var:   0x0000000001e6d2a0
```

Hence, we have a rough idea of where each of these segments are located. As expected,
the stack segment has a high address while the data and heap segments are much lower
since they follow the text segment. This is exactly what we would expect from a Unix
process' virtual address space. However, we can do much better than this.

Using the /proc filesystem, we can print out the range of virtual addresses for each
segment in the process. All we have to do is modify main.c slightly.

```c
#include <stdio.h>
#include <stdlib.h>

static int static_var;
int global_var;

int		main(void)
{
	int		stack_var = 0;
	int		*heap_var;

	heap_var = malloc(sizeof(int));
	printf("static_var: %p\nglobal_var: %p\n", &static_var, &global_var);
	printf("stack_var: %p\nheap_var: %p\n", &stack_var, heap_var);
	free(heap_var);

	/*
	 * We infinitely loop so that we can use the /proc filesystem
	 * to analyze this process after running it.
	 */
	for (;;);

	return 0;
```

Now that our process infinitely loops, we can analyze its data via
/proc.

```txt
# gcc main.c -o a.out.loop
# ./a.out.loop &
# ps -f | grep a.out.loop
elliot    5679  1645 99 17:19 pts/0    00:00:32 ./a.out.loop

# cat /proc/5679/maps
00400000-00401000 r--p 00000000 fd:02 12984484                /home/elliot/projects/linux-internals-simplified/address_space/a.out.loop
00401000-00402000 r-xp 00001000 fd:02 12984484                /home/elliot/projects/linux-internals-simplified/address_space/a.out.loop
00402000-00403000 r--p 00002000 fd:02 12984484                /home/elliot/projects/linux-internals-simplified/address_space/a.out.loop
00403000-00404000 r--p 00002000 fd:02 12984484                /home/elliot/projects/linux-internals-simplified/address_space/a.out.loop
00404000-00405000 rw-p 00003000 fd:02 12984484                /home/elliot/projects/linux-internals-simplified/address_space/a.out.loop
0214d000-0216e000 rw-p 00000000 00:00 0                       [heap]
7f1e8143a000-7f1e8145c000 r--p 00000000 fd:00 3278046         /usr/lib64/libc-2.29.so
7f1e8145c000-7f1e815a9000 r-xp 00022000 fd:00 3278046         /usr/lib64/libc-2.29.so
7f1e815a9000-7f1e815f5000 r--p 0016f000 fd:00 3278046         /usr/lib64/libc-2.29.so
7f1e815f5000-7f1e815f6000 ---p 001bb000 fd:00 3278046         /usr/lib64/libc-2.29.so
7f1e815f6000-7f1e815fa000 r--p 001bb000 fd:00 3278046         /usr/lib64/libc-2.29.so
7f1e815fa000-7f1e815fc000 rw-p 001bf000 fd:00 3278046         /usr/lib64/libc-2.29.so
7f1e815fc000-7f1e81602000 rw-p 00000000 00:00 0 
7f1e81623000-7f1e81624000 r--p 00000000 fd:00 3277967         /usr/lib64/ld-2.29.so
7f1e81624000-7f1e81644000 r-xp 00001000 fd:00 3277967         /usr/lib64/ld-2.29.so
7f1e81644000-7f1e8164c000 r--p 00021000 fd:00 3277967         /usr/lib64/ld-2.29.so
7f1e8164d000-7f1e8164e000 r--p 00029000 fd:00 3277967         /usr/lib64/ld-2.29.so
7f1e8164e000-7f1e8164f000 rw-p 0002a000 fd:00 3277967         /usr/lib64/ld-2.29.so
7f1e8164f000-7f1e81650000 rw-p 00000000 00:00 0 
7ffcbb1ab000-7ffcbb1cd000 rw-p 00000000 00:00 0               [stack]
7ffcbb1df000-7ffcbb1e3000 r--p 00000000 00:00 0               [vvar]
7ffcbb1e3000-7ffcbb1e5000 r-xp 00000000 00:00 0               [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0       [vsyscall]

# kill 5679
```

If we just look at the first five lines of the output:

```txt
    VA Range      PERM                                                        Source File
00400000-00401000 r--p 00000000 fd:02 12984484     /home/elliot/projects/linux-internals-simplified/address_space/a.out.loop
00401000-00402000 r-xp 00001000 fd:02 12984484     /home/elliot/projects/linux-internals-simplified/address_space/a.out.loop
00402000-00403000 r--p 00002000 fd:02 12984484     /home/elliot/projects/linux-internals-simplified/address_space/a.out.loop
00403000-00404000 r--p 00002000 fd:02 12984484     /home/elliot/projects/linux-internals-simplified/address_space/a.out.loop
00404000-00405000 rw-p 00003000 fd:02 12984484     /home/elliot/projects/linux-internals-simplified/address_space/a.out.loop

# ./a.out | head -2
static_var: 0x404038
global_var: 0x40403c
```

We will find each segment constructed from the executable image of a.out.loop, whose full path is
provided in the rightmost column. Reading the first two columns from the left, we can see the
range of virtual addresses along with their permissions (read, write, execute).

Because of address randomization, the location of the variables from a.out and a.out.loop might
be slightly different. However, we can see that in general the static and global variables
are contained in the 00404000-00405000 range whose permissions are rw.

What about the stack and heap variables? Since these segments are essentially scrap paper to the
process, they do not need to be created from a file! Instead, these segments are created as
segments of _anonymous memory_, which is just a fancy way of saying zero-filled memory that
does not belong to any file and does NOT need to be written to disk after its been modified.

To be more precise, the stack and heap segments can be swapped out of memory by the paging
daemon for memory management purposes, but they will never be written to disk to be used 
by another process once our current process exits. The pages that make up these segments
will simply be cleared and freed for use by other processes.

The nature of these two segments is apparent when we observe their entries in the /proc printout:

```txt
00000214d000-00000216e000 rw-p 00000000 00:00 0     [heap]
7ffcbb1ab000-7ffcbb1cd000 rw-p 00000000 00:00 0     [stack]
```

Notice that instead of a source file, these segments are simply identified as what they are.
Given that the heap grows from the end of the executable image towards the stack it makes sense
that its range is 0214d000-0216e000, while the stack which grows down from almost the top of
the virtual address space (the stack is beneath the vvar, vsdo, and vsyscall) is within the
range 7ffcbb1ab000-7ffcbb1cd000.

One final comment before we expand our understanding to include kernel space, we often learn
that shared libraries are stored between the stack and the heap such that both of those
regions grow towards them. In other words, the heap grows up towards the libraries, while the
stack grows down towards the libraries. If you look at the segments in ascending order,
you can get a rough idea of how large each of segments can grow in virtual space!

```txt
        VA Range          PERM                               Source File                  Comments
00000214d000-00000216e000 rw-p 00000000 00:00 0        [heap]                  7f1e8143a000 - 0216e000 = 127TiB
7f1e8143a000-7f1e8145c000 r--p 00000000 fd:00 3278046  /usr/lib64/libc-2.29.so   
7f1e8145c000-7f1e815a9000 r-xp 00022000 fd:00 3278046  /usr/lib64/libc-2.29.so
7f1e815a9000-7f1e815f5000 r--p 0016f000 fd:00 3278046  /usr/lib64/libc-2.29.so
7f1e815f5000-7f1e815f6000 ---p 001bb000 fd:00 3278046  /usr/lib64/libc-2.29.so
7f1e815f6000-7f1e815fa000 r--p 001bb000 fd:00 3278046  /usr/lib64/libc-2.29.so
7f1e815fa000-7f1e815fc000 rw-p 001bf000 fd:00 3278046  /usr/lib64/libc-2.29.so
7ffcbb1ab000-7ffcbb1cd000 rw-p 00000000 00:00 0        [stack]                 7ffcbb1cd000 - 7f1e815fc000 = 888Gib
```

Hence, the potential sizes of these segments on 64bit machines are ENORMOUS! Of course, we
aren't taking into account red zones or process rlimits here, but it's fun to see just how
far we've come from 32-bit OSes. :)

Now that we have seen how to analyze the user segments of the process's virtual address
space, - or in other words, _userspace_ - it is time to broaden our perspective to encompass
_kernel space_ as well. We can use the _/proc_ filesystem to analyze the kernel's segments
in much the same way as we did with our process. The only difference is that we will
obtain the ranges by using symbols.

```txt
# sudo cat /proc/kallsyms | grep _text | head -1
ffffffff92000000 T _text

# sudo cat /proc/kallsyms | grep _etext | head -1
ffffffff92e00ed1 T _etext

# sudo cat /proc/kallsyms | grep _sdata | head -1

ffffffff93600000 D _sdata

# sudo cat /proc/kallsyms | grep _edata | head -1
ffffffff93858400 D _edata

# sudo cat /proc/kallsyms | grep __bss_start | head -1
ffffffff94204000 B __bss_start

# sudo cat /proc/kallsyms | grep __bss_stop | head -1
ffffffff94800000 B __bss_stop

# sudo cat /proc/kallsyms | grep B\ _end
ffffffff9482c000 B _end
```

Given that the mapping of a virtual address in kernel space to its physical address
is simply `pa = va - __PAGE_OFFSET`, we can easily determine the physical locations
of all the kernel segments.

```txt
For __PAGE_OFFSET = 0xffff888000000000 ...

_text: 0x777F92000000

_etext: 0x777F92E00ED1

_sdata: 0x777F93600000

_edata: 0x777F93858400

__bss_start: 0x777F94204000

__bss_stop: 0x777F94800000

_end: 0x777F9482C000
```

Putting everything we learned together, we can create a visualization of
our program's virtual address space!

```txt
_____________________ 0xffffffff9482c000 _end
|XXXXXXXXXXXXXXXXXXX|
|-------------------| 0xffffffff94800000
|                   |
| __bss             |
|___________________| 0xffffffff94204000
|XXXXXXXXXXXXXXXXXXX|
|-------------------| 0xffffffff93858400
|                   |
| _data             |
|___________________| 0xffffffff93600000
|XXXXXXXXXXXXXXXXXXX|
|-------------------| 0xffffffff92e00ed1
|                   |
| _text             |
|___________________| 0xffffffff92000000
|XXXXXXXXXXXXXXXXXXX|
|-------------------| 0xffffffffff601000
|                   |
| vsyscall          |
|___________________| 0xffffffffff600000
|XXXXXXXXXXXXXXXXXXX|
|-------------------| 0x00007ffcbb1e5000
|                   |
| vdso              |
|___________________| 0x00007ffcbb1e3000
|                   |
| vvar              |
|___________________| 0x00007ffcbb1df000
|XXXXXXXXXXXXXXXXXXX|
|-------------------| 0x00007ffcbb1cd000
|                   |
| Stack             |
|___________________| 0x00007ffcbb1ab000
|XXXXXXXXXXXXXXXXXXX|
|-------------------| 0x00007f1e81650000
|                   |
| unknown           |
|___________________| 0x00007f1e8164f000
|                   |
| ld-2.29.so data   |
|___________________| 0x00007f1e8164e000
|                   |
| ld-2.29.so        |
|___________________| 0x00007f1e8164d000
|XXXXXXXXXXXXXXXXXXX|
|-------------------| 0x00007f1e8164c000
|                   |
| ld-2.29.so        |
|___________________| 0x00007f1e81644000
|                   |
| ld-2.29.so text   |
|___________________| 0x00007f1e81624000
|                   |
| ld-2.29.so        |
|___________________| 0x00007f1e81623000
|XXXXXXXXXXXXXXXXXXX|
|-------------------| 0x00007f1e81602000
|                   |
| unknown           |
|___________________| 0x00007f1e815fc000
|                   |
| libc-2.29.so data |
|___________________| 0x00007f1e815fa000
|                   |
| libc-2.29.so      |
|___________________| 0x00007f1e815f6000
|                   |
| libc-2.29.so      |
|___________________| 0x00007f1e815f5000
|                   |
| libc-2.29.so      |
|___________________| 0x00007f1e815a9000
|                   |
| libc-2.29.so text |
|___________________| 0x00007f1e8145c000
|                   |
| libc-2.29.so      |
|___________________| 0x00007f1e8143a000
|                   |
| heap              |
|___________________| 0x000000000214d000
|XXXXXXXXXXXXXXXXXXX|
|-------------------| 0x0000000000405000
|                   |
| _sdata            |
|___________________| 0x0000000000404000
|                   |
| unknown           |
|___________________| 0x0000000000403000
|                   |
| unknown           |
|___________________| 0x0000000000402000
|                   |
| _text             |
|___________________| 0x0000000000401000
|                   |
| unknown           |
|___________________| 0x0000000000400000
|                   |
| NULL ref catch    |
|___________________| 0x0
```
