# Linux Kernel Internals Simplified: Running Processes in the Shell 1

For this set of notes we will continue to trace through and explore
the process of running a program on the bash shell. To briefly
review what we covered in the last section:

1. Used _strace_ on a bash shell
2. Executed the command `./a.out` in the traced bash shell
3. Saw how _pselect6_ is used to read/write keyboard strokes into the shell
4. Saw how _ioctl_ is used to modify the terminal settings for the child process
5. Saw how *rt_sigaction* and *rt_sigprocmask* modify the signal handlers and blocked signals for the child process
6. Saw how _pipe_ was used to create a unidirectional pipe
7. Saw how _clone_ was used to create the child process with no stack to execute the `./a.out` command
8. Saw how _wait4_ was used to wait for the child process to change its state

Now it is time to see how the child process becomes the a.out program and
finally executes our shell command. In order to trace this, we must use
_strace_ again.

```txt
# strace -o cmd_trace.txt ./a.out

# cat cmd_trace.txt
execve("./a.out", ["./a.out"], 0x7ffddc9d4f70 /* 67 vars */) = 0
brk(NULL)                               = 0x1b8c000
arch_prctl(0x3001 /* ARCH_??? */, 0x7fff81ad44c0) = -1 EINVAL (Invalid argument)
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=131634, ...}) = 0
mmap(NULL, 131634, PROT_READ, MAP_PRIVATE, 3, 0) = 0x7ffa6dd9c000
close(3)                                = 0
openat(AT_FDCWD, "/lib64/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0P@\2\0\0\0\0\0"..., 832) = 832
lseek(3, 792, SEEK_SET)                 = 792
read(3, "\4\0\0\0\24\0\0\0\3\0\0\0GNU\0Q7r6\255\1\200\216&@L\230\372\244\35r"..., 68) = 68
fstat(3, {st_mode=S_IFREG|0755, st_size=5589976, ...}) = 0
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7ffa6dd9a000
lseek(3, 792, SEEK_SET)                 = 792
read(3, "\4\0\0\0\24\0\0\0\3\0\0\0GNU\0Q7r6\255\1\200\216&@L\230\372\244\35r"..., 68) = 68
lseek(3, 864, SEEK_SET)                 = 864
read(3, "\4\0\0\0\20\0\0\0\5\0\0\0GNU\0\2\0\0\300\4\0\0\0\3\0\0\0\0\0\0\0", 32) = 32
mmap(NULL, 1857280, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7ffa6dbd4000
mprotect(0x7ffa6dbf6000, 1679360, PROT_NONE) = 0
mmap(0x7ffa6dbf6000, 1363968, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x22000) = 0x7ffa6dbf6000
mmap(0x7ffa6dd43000, 311296, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x16f000) = 0x7ffa6dd43000
mmap(0x7ffa6dd90000, 24576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1bb000) = 0x7ffa6dd90000
mmap(0x7ffa6dd96000, 14080, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x7ffa6dd96000
close(3)                                = 0
arch_prctl(ARCH_SET_FS, 0x7ffa6dd9b500) = 0
mprotect(0x7ffa6dd90000, 16384, PROT_READ) = 0
mprotect(0x403000, 4096, PROT_READ)     = 0
mprotect(0x7ffa6dde7000, 4096, PROT_READ) = 0
munmap(0x7ffa6dd9c000, 131634)          = 0
write(1, "Hello standard output!\n", 23) = 23
exit_group(23)                          = ?
+++ exited with 23 +++
```

This 33 line trace might appear intimidating at first, but most of the system
calls are very familiar to kernel enthusiasts. For example, _execve_ to
overwrite the child process's image with a.out, _fstat_ to acquire file
metadata, _mmap_ to map files into the child process's address space, and
so on. Despite this familiarity however, we will continue to analyze man
pages to understand the in/out behavior of all of these functions. Only
after understanding this we will begin to dive into the souce code.

Let's start from the top with _execve_ call!

```txt
EXECVE(2)                Linux Programmer's Manual                               EXECVE(2)

NAME
       execve - execute program

SYNOPSIS
       int execve(const char *filename, char *const argv[],
                  char *const envp[]);

DESCRIPTION
       execve()  executes  the program pointed to by filename.  This causes the program
       that is currently being run by the calling process to be replaced with a new
       program, with newly initialized stack, heap, and (initialized and uninitialized)
       data segments.

       filename must be either a binary executable, or a script starting with a line of
       the form:

           #! interpreter [optional-arg]

       argv is an array of argument strings passed to the new program. By convention,
       the first of these strings (i.e., argv[0]) should contain the filename associated
       with the file being executed. envp is an array of strings, conventionally of the
       form key=value, which are passed as environment to the new program. The argv and
       envp arrays must each include a null pointer at the end of the array.

       The argument vector and environment can be accessed by the called program's main
       function, when it is defined as:

           int main(int argc, char *argv[], char *envp[])

       Note, however, that the use of a third argument to the main function is not specified
       in POSIX.1; according to POSIX.1, the environment should be accessed via the
       external variable environ(7).

       execve() does not return on success, and the text, initialized data, uninitialized
       data (bss), and stack of the calling process are overwritten according to the contents
       of the newly loaded program.

       If the executable is a dynamically linked ELF executable, the interpreter named in the
       PT_INTERP segment is used to load the needed shared objects. This interpreter is typically
       /lib/ld-linux.so.2 for binaries linked with glibc (see ld-linux.so(8)).

RETURN VALUE
       On success, execve() does not return, on error -1 is returned, and errno is set appropriately.

Linux                    2018-04-30                                              EXECVE(2)
```

Hence, the _execve_ call overwrites the child process's text, initialized and
uninitialized data, and stack with a.out's as expected. The man page also
states that ELF binaries are typically linked with /lib/ld-linux.so.2, but
this appears to be outdated.

```txt
# cat cmd_trace.txt | head -1
execve("./a.out", ["./a.out"], 0x7ffddc9d4f70 /* 67 vars */) = 0

# file a.out
a.out: ELF 64-bit LSB executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, for GNU/Linux 3.2.0, BuildID[sha1]=9e94a6e62c98386babf2beaef924fbe47b208636, not stripped

# readelf -l a.out

Elf file type is EXEC (Executable file)
Entry point 0x401040
There are 11 program headers, starting at offset 64

Program Headers:
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz              Flags  Align
  PHDR           0x0000000000000040 0x0000000000400040 0x0000000000400040
                 0x0000000000000268 0x0000000000000268  R      0x8
  INTERP         0x00000000000002a8 0x00000000004002a8 0x00000000004002a8
                 0x000000000000001c 0x000000000000001c  R      0x1
      [Requesting program interpreter: /lib64/ld-linux-x86-64.so.2]
  LOAD           0x0000000000000000 0x0000000000400000 0x0000000000400000
                 0x0000000000000438 0x0000000000000438  R      0x1000
  LOAD           0x0000000000001000 0x0000000000401000 0x0000000000401000
                 0x00000000000001d5 0x00000000000001d5  R E    0x1000
  LOAD           0x0000000000002000 0x0000000000402000 0x0000000000402000
                 0x0000000000000150 0x0000000000000150  R      0x1000
  LOAD           0x0000000000002e10 0x0000000000403e10 0x0000000000403e10
                 0x0000000000000214 0x0000000000000218  RW     0x1000
  DYNAMIC        0x0000000000002e20 0x0000000000403e20 0x0000000000403e20
                 0x00000000000001d0 0x00000000000001d0  RW     0x8
  NOTE           0x00000000000002c4 0x00000000004002c4 0x00000000004002c4
                 0x0000000000000044 0x0000000000000044  R      0x4
  GNU_EH_FRAME   0x0000000000002028 0x0000000000402028 0x0000000000402028
                 0x000000000000003c 0x000000000000003c  R      0x4
  GNU_STACK      0x0000000000000000 0x0000000000000000 0x0000000000000000
                 0x0000000000000000 0x0000000000000000  RW     0x10
  GNU_RELRO      0x0000000000002e10 0x0000000000403e10 0x0000000000403e10
                 0x00000000000001f0 0x00000000000001f0  R      0x1

 Section to Segment mapping:
  Segment Sections...
   00     
   01     .interp 
   02     .interp .note.ABI-tag .note.gnu.build-id .gnu.hash .dynsym .dynstr .gnu.version .gnu.version_r .rela.dyn .rela.plt 
   03     .init .plt .text .fini 
   04     .rodata .eh_frame_hdr .eh_frame 
   05     .init_array .fini_array .dynamic .got .got.plt .data .bss 
   06     .dynamic 
   07     .note.ABI-tag .note.gnu.build-id 
   08     .eh_frame_hdr 
   09     
   10     .init_array .fini_array .dynamic .got 
```

If you look at the above code block, we can see the _execve_ call with its
arguments: the path of the executable "./a.out", the argv ["./a.out"],
and the envp. Furthermore, the _file_ command reveals that this binary is
dynamically linked with /lib64/ld-linux-x86-64.so.2, not ld-linux.so.2.
This is also confirmed by _readelf_ as well.

Now let's move on to _brk_.

```txt
BRK(2)                   Linux Programmer's Manual                               BRK(2)

NAME
       brk, sbrk - change data segment size

SYNOPSIS
       int brk(void *addr);

       void *sbrk(intptr_t increment);

DESCRIPTION
       brk() and sbrk() change the location of the program break, which defines the end of
       the process's data segment (i.e., the program break is the first location after
       the end of the uninitialized data segment). Increasing the program break has the
       effect of allocating memory to the process; decreasing the break deallocates memory.

       brk() sets the end of the data segment to the value specified by addr, when that value
       is reasonable, the system has enough memory, and the process does not exceed its maximum
       data size (see setrlimit(2)).

       sbrk() increments the program's data space by increment bytes. Calling sbrk() with an
       increment of 0 can be used to find the current location of the program break.

RETURN VALUE
       On success, brk() returns zero. On error, -1 is returned, and errno is set to ENOMEM.

       On success, sbrk() returns the previous program break. (If the break was increased,
       then this value is a pointer to the start of the newly allocated memory). On error,
       (void *) -1 is returned, and errno is set to ENOMEM.
NOTES
   C library/kernel differences
       The return value described above for brk() is the behavior provided by the glibc wrapper
       function for the Linux brk() system call. (On most other  implementations, the return
       value from brk() is the same; this return value was also specified in SUSv2.) However,
       the actual Linux system call returns the new program break on success. On failure, the
       system call returns the current break.

Linux                    2016-03-15                                              BRK(2)
```

Based upon the man page for _brk_, the return value in the trace is current
location of the break since we passed a bogus argument to the call. This
illustrates an important point that all of the system calls listed in _strace_
are the actual Linux system calls and NOT glibc wrappers.

```txt
brk(NULL)                               = 0x1b8c000
```

Let's look at the next six lines of the trace.

```txt
# cat cmd_trace.txt | head -8 | tail -6
arch_prctl(0x3001 /* ARCH_??? */, 0x7fff81ad44c0) = -1 EINVAL (Invalid argument)
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=131634, ...}) = 0
mmap(NULL, 131634, PROT_READ, MAP_PRIVATE, 3, 0) = 0x7ffa6dd9c000
close(3)                                = 0
```

Both the *arch_prctl* and _access_ system calls fail for different reasons,
with _access's_ error being immediately more obvious to understand.

```txt
# ls /etc/ | grep ^ld
ld.so.cache
ld.so.conf
ld.so.conf.d
```

As one would expect from ENOENT, the linker file ld.so.preload does not exist!
So whether or not we can access its path is irrelevant. As for *arch_prctl*...

```txt
ARCH_PRCTL(2)            Linux Programmer's Manual                               ARCH_PRCTL(2)

NAME
       arch_prctl - set architecture-specific thread state

SYNOPSIS
       int arch_prctl(int code, unsigned long addr);
       int arch_prctl(int code, unsigned long *addr);

DESCRIPTION
       arch_prctl() sets architecture-specific process or thread state. code selects a
       subfunction and passes argument addr to it; addr is interpreted as either an
       unsigned long for the "set" operations, or as an unsigned long *, for the "get"
       operations.

       Subfunctions for x86-64 are:

       ARCH_SET_FS
              Set the 64-bit base for the FS register to addr.

       ARCH_GET_FS
              Return the 64-bit base value for the FS register of the current thread in the
              unsigned long pointed to by addr.

       ARCH_SET_GS
              Set the 64-bit base for the GS register to addr.

       ARCH_GET_GS
              Return the 64-bit base value for the GS register of the current thread in the
              unsigned long pointed to by addr.

RETURN VALUE
       On success, arch_prctl() returns 0; on error, -1 is returned, and errno is set to
       indicate the error.

ERRORS
       EFAULT addr points to an unmapped address or is outside the process address space.

       EINVAL code is not a valid subcommand.

       EPERM  addr is outside the process address space.

Linux                    2017-09-15                                              ARCH_PRCTL(2)
```

We merely learn that the address 0x3001 is not a valid subcommand for x86-64.
By subcommand, we mean it is not an instruction to interact with the FS and GS
registers.

The next four functions all work together to map the cached linker object
ld.so.cache into the child process's address space. This can be understood
without reading man pages by simply observing the arguments.

```txt
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=131634, ...}) = 0
mmap(NULL, 131634, PROT_READ, MAP_PRIVATE, 3, 0) = 0x7ffa6dd9c000
close(3)                                = 0
```

We use _openat_ system call to open the cached linker file, _fstat_ to acquire
its file size of st\_size=131634, _mmap_ to create a read only private mapping
of 131634 bytes, and finally _close_ to close linker file's descriptor. Of these
four functions, we will only study _mmap_ in extreme detail because of the function's
ubiquity in the trace and indeed many of the system calls whose implementation
we will study in later notes.

```txt
MMAP(2)                  Linux Programmer's Manual                               MMAP(2)

NAME
       mmap, munmap - map or unmap files or devices into memory

SYNOPSIS
       void *mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset);
       int munmap(void *addr, size_t length);

DESCRIPTION
       mmap() creates a new mapping in the virtual address space of the calling process.
       The starting address for the new mapping is specified in addr. The length argument
       specifies the length of the mapping (which must be greater than 0).

       If addr is NULL, then the kernel chooses the (page-aligned) address at which to
       create the mapping; this is the most portable method of creating a  new  mapping.
       If addr is not  NULL, then the kernel takes it as a hint about where to place the
       mapping; on Linux, the mapping will be created at a nearby page boundary. The
       address of the new mapping is returned as the result of the call.

       The contents of a file mapping (as opposed to an anonymous mapping; see MAP_ANONYMOUS
       below), are initialized using length bytes starting at offset offset in the file
       (or other object) referred to by the file descriptor fd. offset must be a multiple
       of the page size as returned by sysconf(_SC_PAGE_SIZE).

       The prot argument describes the desired memory protection of the mapping (and must
       not conflict with the open mode of the file). It is either PROT_NONE or the bitwise
       OR of one or more of the following flags:

       PROT_EXEC  Pages may be executed.

       PROT_READ  Pages may be read.

       PROT_WRITE Pages may be written.

       PROT_NONE  Pages may not be accessed.

       The flags argument determines whether updates to the mapping are visible to other
       processes mapping the same region, and whether updates are carried  through to
       the underlying file. This behavior is determined by including exactly one of the
       following values in flags:

       MAP_PRIVATE
              Create a private copy-on-write mapping. Updates to the mapping are not
              visible to other processes mapping the same file, and are not carried through
              to the underlying file. It is unspecified whether changes made to the file
              after the mmap() call are visible in the mapped region.

       In addition, zero or more of the following values can be ORed in flags:

       MAP_ANONYMOUS
              The mapping is not backed by any file; its contents are initialized to zero.
              The fd argument is ignored; however, some implementations require fd to be -1
              if MAP_ANONYMOUS (or  MAP_ANON) is specified, and portable applications should
              ensure this. The offset argument should be zero. The use of MAP_ANONYMOUS in
              conjunction with MAP_SHARED is supported on Linux only since kernel 2.4.

       MAP_DENYWRITE
              This flag is ignored. (Long ago—Linux 2.0 and earlier—it signaled that attempts
              to write to the underlying file should fail with ETXTBUSY. But this was a source
              of denial-of-service attacks.)

       MAP_FIXED
              Don't interpret addr as a hint: place the mapping at exactly that address. addr
              must be suitably aligned: for most architectures a multiple of the page size is
              sufficient; however, some architectures may impose additional restrictions. If
              the memory region specified by addr and len overlaps pages of any existing
              mapping(s), then the overlapped part of the existing mapping(s) will be discarded.
              If the specified address cannot be used, mmap() will fail.

              Software that aspires to be portable should use the MAP_FIXED flag with care,
              keeping in mind that the exact layout of a process's memory mappings is allowed to
              change significantly between kernel versions, C library versions, and operating
              system releases. Carefully read the discussion of this flag in NOTES!

       Memory mapped by mmap() is preserved across fork(2), with the same attributes.

       A file is mapped in multiples of the page size. For a file that is not a multiple of the
       page size, the remaining memory is zeroed when mapped, and writes to that region are not
       written out to the file.The effect of changing the size of the underlying file of a
       mapping on the pages that correspond to added or removed regions of the file is unspecified.

RETURN VALUE
       On success, mmap() returns a pointer to the mapped area. On error, the value MAP_FAILED
       (that is, (void *) -1) is returned, and errno is set to indicate the cause of the error.

       On success, munmap() returns 0. On failure, it returns -1, and errno is set to indicate the
       cause of the error (probably to EINVAL).

ATTRIBUTES
       For an explanation of the terms used in this section, see attributes(7).

       ┌───────────────────┬───────────────┬─────────┐
       │Interface          │ Attribute     │ Value   │
       ├───────────────────┼───────────────┼─────────┤
       │mmap(), munmap()   │ Thread safety │ MT-Safe │
       └───────────────────┴───────────────┴─────────┘

Linux                    2018-04-30                                              MMAP(2)
``` 

Now that we are familiar with the process of mapping a file into the child
process's address space, we can quickly look through the next 17 lines of the
trace.

```txt
# cat cmd_trace.txt | head -25 | tail -17
openat(AT_FDCWD, "/lib64/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0P@\2\0\0\0\0\0"..., 832) = 832
lseek(3, 792, SEEK_SET)                 = 792
read(3, "\4\0\0\0\24\0\0\0\3\0\0\0GNU\0Q7r6\255\1\200\216&@L\230\372\244\35r"..., 68) = 68
fstat(3, {st_mode=S_IFREG|0755, st_size=5589976, ...}) = 0
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7ffa6dd9a000	// Create kernel stack?
lseek(3, 792, SEEK_SET)                 = 792
read(3, "\4\0\0\0\24\0\0\0\3\0\0\0GNU\0Q7r6\255\1\200\216&@L\230\372\244\35r"..., 68) = 68
lseek(3, 864, SEEK_SET)                 = 864
read(3, "\4\0\0\0\20\0\0\0\5\0\0\0GNU\0\2\0\0\300\4\0\0\0\3\0\0\0\0\0\0\0", 32) = 32
mmap(NULL, 1857280, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7ffa6dbd4000
mprotect(0x7ffa6dbf6000, 1679360, PROT_NONE) = 0
mmap(0x7ffa6dbf6000, 1363968, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x22000) = 0x7ffa6dbf6000
mmap(0x7ffa6dd43000, 311296, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x16f000) = 0x7ffa6dd43000
mmap(0x7ffa6dd90000, 24576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1bb000) = 0x7ffa6dd90000
mmap(0x7ffa6dd96000, 14080, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x7ffa6dd96000
close(3)                                = 0
```

Perhaps the most mysterious aspect of these system calls is the 68 byte read
from offet 792 (0x318) of /lib64/libc.so.6. This read is made twice, but it
is unclear from trace what this data is used for or why it needs to be read
more than once. Following that, _mmap_ is used to create five mappings from
/lib64/libc.so.6, where the purpose of two read-only mappings is unclear.

Although the purpose of some of these mappings are unknown, they are very
reminiscent of the mappings we saw in the address\_space notes. Perhaps
they will only be clear after examining the internals of the linker
/lib64/ld-linux-x86-64.so.2.

For now though, we will leave those questions for source code analysis and
move on to the last 8 lines of the trace.

```txt
# cat cmd_trace.txt | tail -8
arch_prctl(ARCH_SET_FS, 0x7ffa6dd9b500) = 0
mprotect(0x7ffa6dd90000, 16384, PROT_READ) = 0
mprotect(0x403000, 4096, PROT_READ)     = 0
mprotect(0x7ffa6dde7000, 4096, PROT_READ) = 0
munmap(0x7ffa6dd9c000, 131634)          = 0
write(1, "Hello standard output!\n", 23) = 23
exit_group(23)                          = ?
+++ exited with 23 +++
```

Recall that *arch_prctl* merely calls a subcommand to set the value of
either FS or GS. Thus, the call here sets FS = 0x7ffa6dd9b500.

The *mprotect* call is used to change the protections on memory mappings,
and makes perfect sense with a quick look at its man page.

```txt
MPROTECT(2)              Linux Programmer's Manual                               MPROTECT(2)

NAME
       mprotect, pkey_mprotect - set protection on a region of memory

SYNOPSIS
       int mprotect(void *addr, size_t len, int prot);
       int pkey_mprotect(void *addr, size_t len, int prot, int pkey);

DESCRIPTION
       mprotect() changes the access protections for the calling process's memory pages
       containing any part of the address range in the interval [addr, addr+len-1].
       addr must be aligned to a page boundary.

       If the calling process tries to access memory in a manner that violates the
       protections, then the kernel generates a SIGSEGV signal for the process.

       prot is a combination of the following access flags: PROT_NONE or a bitwise-or
       of the other values in the following list:

       PROT_NONE  The memory cannot be accessed at all.

       PROT_READ  The memory can be read.

       PROT_WRITE The memory can be modified.

       PROT_EXEC  The memory can be executed.

RETURN VALUE
       On success, mprotect() and pkey_mprotect() return zero.  On error, these system
       calls return -1, and errno is set appropriately.

NOTES
       On Linux, it is always permissible to call mprotect() on any address in a process's
       address space (except for the kernel vsyscall area). In particular, it can be used to
       change existing code mappings to be writable.

Linux                    2018-02-02                                              MPROTECT(2)
```

After changing various protections and unmapping a region, we finally get to
_write_ system call from our main.c program. Since the main function returns
the number of characters written in "Hello standard output!\n", the process
calls *exit_group* with a value of 23. This call exits every thread in the
calling process, which in our case is just process 5603.

```txt
EXIT_GROUP(2)            Linux Programmer's Manual                               EXIT_GROUP(2)

NAME
       exit_group - exit all threads in a process

SYNOPSIS
       void exit_group(int status);

DESCRIPTION
       This system call is equivalent to _exit(2) except that it terminates not only the
       calling thread, but all threads in the calling process's thread group.

RETURN VALUE
       This system call does not return.

VERSIONS
       This call is present since Linux 2.5.35.

CONFORMING TO
       This call is Linux-specific.

NOTES
       Since glibc 2.3, this is the system call invoked when the _exit(2) wrapper function is called.

Linux                    2008-11-27                                              EXIT_GROUP(2)
```

Following *exit_group*, the rest of the execution picks up where we left off in
the previous set of notes. However, since the rest of the trace is specific to
the machinations of bash and is complex enough for its own set of notes, we will
only list the rest of the trace here before moving on to the source code analysis
of the process related system calls used in the `./a.out` command.

```txt
# cat shell_trace.txt | tail -91
wait4(-1, [{WIFEXITED(s) && WEXITSTATUS(s) == 23}], WSTOPPED|WCONTINUED, NULL) = 5603
rt_sigprocmask(SIG_BLOCK, [CHLD TSTP TTIN TTOU], [CHLD], 8) = 0
ioctl(255, TIOCSPGRP, [5451])           = 0
rt_sigprocmask(SIG_SETMASK, [CHLD], NULL, 8) = 0
ioctl(255, TCGETS, {B38400 opost isig icanon echo ...}) = 0
ioctl(255, TIOCGWINSZ, {ws_row=36, ws_col=169, ws_xpixel=0, ws_ypixel=0}) = 0
rt_sigprocmask(SIG_SETMASK, [], NULL, 8) = 0
--- SIGCHLD {si_signo=SIGCHLD, si_code=CLD_EXITED, si_pid=5603, si_uid=1000, si_status=23, si_utime=0, si_stime=0} ---
wait4(-1, 0x7ffcd471c990, WNOHANG|WSTOPPED|WCONTINUED, NULL) = -1 ECHILD (No child processes)
rt_sigreturn({mask=[]})                 = 0
rt_sigaction(SIGINT, {sa_handler=0x5584c8f03e60, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f03e60, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigprocmask(SIG_BLOCK, NULL, [], 8)  = 0
rt_sigprocmask(SIG_BLOCK, NULL, [], 8)  = 0
pipe([4, 5])                            = 0
rt_sigprocmask(SIG_BLOCK, [CHLD], [], 8) = 0
rt_sigprocmask(SIG_SETMASK, [], NULL, 8) = 0
rt_sigprocmask(SIG_BLOCK, [INT CHLD], [], 8) = 0
pipe([6, 7])                            = 0
clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD, child_tidptr=0x7f3a7242ca10) = 5604
setpgid(5604, 5451)                     = 0
rt_sigprocmask(SIG_SETMASK, [], NULL, 8) = 0
rt_sigaction(SIGCHLD, {sa_handler=0x5584c8ee6d20, sa_mask=[], sa_flags=SA_RESTORER|SA_RESTART, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8ee6d20, sa_mask=[], sa_flags=SA_RESTORER|SA_RESTART, sa_restorer=0x7f3a72466ec0}, 8) = 0
close(6)                                = 0
close(7)                                = 0
close(5)                                = 0
read(4, "./a.out\n", 128)               = 8
read(4, "", 128)                        = 0
--- SIGCHLD {si_signo=SIGCHLD, si_code=CLD_EXITED, si_pid=5604, si_uid=1000, si_status=0, si_utime=0, si_stime=0} ---
wait4(-1, [{WIFEXITED(s) && WEXITSTATUS(s) == 0}], WNOHANG|WSTOPPED|WCONTINUED, NULL) = 5604
wait4(-1, 0x7ffcd471bbd0, WNOHANG|WSTOPPED|WCONTINUED, NULL) = -1 ECHILD (No child processes)
rt_sigreturn({mask=[]})                 = 0
close(4)                                = 0
rt_sigprocmask(SIG_BLOCK, [CHLD], [], 8) = 0
rt_sigprocmask(SIG_BLOCK, [CHLD TSTP TTIN TTOU], [CHLD], 8) = 0
ioctl(255, TIOCSPGRP, [5451])           = 0
rt_sigprocmask(SIG_SETMASK, [CHLD], NULL, 8) = 0
rt_sigprocmask(SIG_SETMASK, [], NULL, 8) = 0
rt_sigprocmask(SIG_BLOCK, NULL, [], 8)  = 0
rt_sigprocmask(SIG_BLOCK, NULL, [], 8)  = 0
pipe([4, 5])                            = 0
rt_sigprocmask(SIG_BLOCK, [CHLD], [], 8) = 0
rt_sigprocmask(SIG_SETMASK, [], NULL, 8) = 0
rt_sigprocmask(SIG_BLOCK, [INT CHLD], [], 8) = 0
pipe([6, 7])                            = 0
clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD, child_tidptr=0x7f3a7242ca10) = 5607
setpgid(5607, 5451)                     = 0
rt_sigprocmask(SIG_SETMASK, [], NULL, 8) = 0
rt_sigaction(SIGCHLD, {sa_handler=0x5584c8ee6d20, sa_mask=[], sa_flags=SA_RESTORER|SA_RESTART, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8ee6d20, sa_mask=[], sa_flags=SA_RESTORER|SA_RESTART, sa_restorer=0x7f3a72466ec0}, 8) = 0
close(6)                                = 0
close(7)                                = 0
close(5)                                = 0
read(4, "\33]7;file://localhost.localdomain"..., 128) = 90
read(4, "", 128)                        = 0
--- SIGCHLD {si_signo=SIGCHLD, si_code=CLD_EXITED, si_pid=5607, si_uid=1000, si_status=0, si_utime=0, si_stime=0} ---
wait4(-1, [{WIFEXITED(s) && WEXITSTATUS(s) == 0}], WNOHANG|WSTOPPED|WCONTINUED, NULL) = 5607
wait4(-1, 0x7ffcd471bf90, WNOHANG|WSTOPPED|WCONTINUED, NULL) = -1 ECHILD (No child processes)
rt_sigreturn({mask=[]})                 = 0
close(4)                                = 0
rt_sigprocmask(SIG_BLOCK, [CHLD], [], 8) = 0
rt_sigprocmask(SIG_BLOCK, [CHLD TSTP TTIN TTOU], [CHLD], 8) = 0
ioctl(255, TIOCSPGRP, [5451])           = 0
rt_sigprocmask(SIG_SETMASK, [CHLD], NULL, 8) = 0
rt_sigprocmask(SIG_SETMASK, [], NULL, 8) = 0
write(1, "\33]777;notify;Command completed;."..., 198) = 198
stat("/var/spool/mail/elliot", {st_mode=S_IFREG|0660, st_size=0, ...}) = 0
rt_sigprocmask(SIG_BLOCK, [CHLD TSTP TTIN TTOU], [], 8) = 0
ioctl(255, TIOCSPGRP, [5451])           = 0
rt_sigprocmask(SIG_SETMASK, [], NULL, 8) = 0
rt_sigaction(SIGINT, {sa_handler=0x5584c8f03e60, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f03e60, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
fcntl(0, F_GETFL)                       = 0x8002 (flags O_RDWR|O_LARGEFILE)
ioctl(0, TIOCGWINSZ, {ws_row=36, ws_col=169, ws_xpixel=0, ws_ypixel=0}) = 0
ioctl(0, TIOCSWINSZ, {ws_row=36, ws_col=169, ws_xpixel=0, ws_ypixel=0}) = 0
ioctl(0, TCGETS, {B38400 opost isig icanon echo ...}) = 0
ioctl(0, SNDCTL_TMR_STOP or TCSETSW, {B38400 opost isig -icanon -echo ...}) = 0
rt_sigprocmask(SIG_BLOCK, [HUP INT QUIT ALRM TERM TSTP TTIN TTOU], [], 8) = 0
rt_sigaction(SIGINT, {sa_handler=0x5584c8f5a150, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f03e60, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGTERM, {sa_handler=0x5584c8f5a150, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f03940, sa_mask=[], sa_flags=SA_RESTORER|SA_RESTART, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGHUP, {sa_handler=0x5584c8f5a150, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f04230, sa_mask=[HUP INT ILL TRAP ABRT BUS FPE USR1 SEGV USR2 PIPE ALRM TERM XCPU XFSZ VTALRM SYS], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGQUIT, {sa_handler=0x5584c8f5a150, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=SIG_IGN, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGQUIT, {sa_handler=SIG_IGN, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f5a150, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGALRM, {sa_handler=0x5584c8f5a150, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f04230, sa_mask=[HUP INT ILL TRAP ABRT BUS FPE USR1 SEGV USR2 PIPE ALRM TERM XCPU XFSZ VTALRM SYS], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGTSTP, {sa_handler=0x5584c8f5a150, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=SIG_IGN, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGTSTP, {sa_handler=SIG_IGN, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f5a150, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGTTOU, {sa_handler=0x5584c8f5a150, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=SIG_IGN, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGTTOU, {sa_handler=SIG_IGN, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f5a150, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGTTIN, {sa_handler=0x5584c8f5a150, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=SIG_IGN, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGTTIN, {sa_handler=SIG_IGN, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f5a150, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigprocmask(SIG_SETMASK, [], NULL, 8) = 0
rt_sigaction(SIGWINCH, {sa_handler=0x5584c8f595c0, sa_mask=[], sa_flags=SA_RESTORER|SA_RESTART, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f03930, sa_mask=[], sa_flags=SA_RESTORER|SA_RESTART, sa_restorer=0x7f3a72466ec0}, 8) = 0
write(2, "\33[1;35melliot@localhost:\33[1;34mp"..., 45) = 45
pselect6(1, [0], NULL, NULL, NULL, {[], 8} <detached ...>
```

In the next set of notes, we will begin the source code analysis of the _clone_
system call that was used to create the child process we traced in this
document.
