# Linux Kernel Internals Simplified: Running Processes in the Shell 1

For this set of notes we will trace through and explore the entire
process of running a program on the bash shell. By doing this, we can
get a nearly comphrehensive look at the Linux kernel's insides and
understand how each subsystem interacts with each other.

We will begin by creating a very simple program that interacts
with standard output. Later on, we will create a program that
interacts with the virtual filesystem (vfs).

```c
#include <unistd.h>

int		main(void)
{
	return syscall(1, 1, "Hello standard output!\n", 23);
}
```

After compiling this simple program, we will create a new tab in
our terminal and do the following:

```txt
# ps -f | grep bash    // To learn the pid of this bash session
elliot    4046  4040  0 18:33 pts/0    00:00:00 bash
elliot    5545  5524  0 19:32 pts/0    00:00:00 /bin/bash -c ( ps -f | grep bash)>/tmp/v19bhfr/1 2>&1
elliot    5552  5545  0 19:32 pts/0    00:00:00 /bin/bash -c ( ps -f | grep bash)>/tmp/v19bhfr/1 2>&1
elliot    5554  5552  0 19:32 pts/0    00:00:00 grep bash

# ps -ef | grep bash  // To learn the pid of the other bash session
elliot    4046  4040  0 18:33 pts/0    00:00:00 bash
elliot    4168  1850  0 18:52 tty2     00:00:00 /bin/bash /usr/bin/brave-browser-stable
elliot    5451  4040  0 19:31 pts/1    00:00:00 bash
elliot    5556  5524  0 19:32 pts/0    00:00:00 /bin/bash -c ( ps -ef | grep bash)>/tmp/v19bhfr/2 2>&1
elliot    5563  5556  0 19:32 pts/0    00:00:00 /bin/bash -c ( ps -ef | grep bash)>/tmp/v19bhfr/2 2>&1
elliot    5565  5563  0 19:32 pts/0    00:00:00 grep bash

# strace -p 5451 -o shell_trace.txt   // Trace the other bash session and run ./a.out on it
strace: Process 5451 attached
^Cstrace: Process 5451 detached

# cat shell_trace.txt | tail -5      // Confirm that the output file contains all the content
rt_sigprocmask(SIG_SETMASK, [], NULL, 8) = 0
rt_sigaction(SIGWINCH, {sa_handler=0x5584c8f595c0, sa_mask=[], sa_flags=SA_RESTORER|SA_RESTART, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f03930, sa_mask=[], sa_flags=SA_RESTORER|SA_RESTART, sa_restorer=0x7f3a72466ec0}, 8) = 0
write(2, "\33[1;35melliot@localhost:\33[1;34mp"..., 45) = 45
pselect6(1, [0], NULL, NULL, NULL, {[], 8} <detached ...>
```

I have included a C++ style comment for each shell command to explain
the purpose behind them. It is important to explain that though that
the last command piped through tail confirms that our trace has all
the information we want by ensuring that it ends with strace detaching
from the other bash session.

The trace file contains an enormous amount of information to digest,
and we will explore each function we see in varying degrees of detail.
Let's look at the first 31 lines of the trace file.

```txt
# cat shell_trace.txt | head -10
pselect6(1, [0], NULL, NULL, NULL, {[], 8}) = 1 (in [0])
read(0, ".", 1)                         = 1
select(1, [0], NULL, [0], {tv_sec=0, tv_usec=0}) = 0 (Timeout)
write(2, ".", 1)                        = 1
pselect6(1, [0], NULL, NULL, NULL, {[], 8}) = 1 (in [0])
read(0, "/", 1)                         = 1
select(1, [0], NULL, [0], {tv_sec=0, tv_usec=0}) = 0 (Timeout)
write(2, "/", 1)                        = 1
pselect6(1, [0], NULL, NULL, NULL, {[], 8}) = 1 (in [0])
read(0, "a", 1)                         = 1
select(1, [0], NULL, [0], {tv_sec=0, tv_usec=0}) = 0 (Timeout)
write(2, "a", 1)                        = 1
pselect6(1, [0], NULL, NULL, NULL, {[], 8}) = 1 (in [0])
read(0, ".", 1)                         = 1
select(1, [0], NULL, [0], {tv_sec=0, tv_usec=0}) = 0 (Timeout)
write(2, ".", 1)                        = 1
pselect6(1, [0], NULL, NULL, NULL, {[], 8}) = 1 (in [0])
read(0, "o", 1)                         = 1
select(1, [0], NULL, [0], {tv_sec=0, tv_usec=0}) = 0 (Timeout)
write(2, "o", 1)                        = 1
pselect6(1, [0], NULL, NULL, NULL, {[], 8}) = 1 (in [0])
read(0, "u", 1)                         = 1
select(1, [0], NULL, [0], {tv_sec=0, tv_usec=0}) = 0 (Timeout)
write(2, "u", 1)                        = 1
pselect6(1, [0], NULL, NULL, NULL, {[], 8}) = 1 (in [0])
read(0, "t", 1)                         = 1
select(1, [0], NULL, [0], {tv_sec=0, tv_usec=0}) = 0 (Timeout)
write(2, "t", 1)                        = 1
pselect6(1, [0], NULL, NULL, NULL, {[], 8}) = 1 (in [0])
read(0, "\r", 1)                        = 1
write(2, "\n", 1)                       = 1
```

_pselect_ is a system call that is used to multiplex I/O for processes,
most notably the command line. It can be easily understood for our
purposes right now by listing a few excerpts from its man page,
which is accessed via `man pselect`.

```txt
SELECT(2)                Linux Programmer's Manual                               SELECT(2)

NAME
       select, pselect, FD_CLR, FD_ISSET, FD_SET, FD_ZERO - synchronous I/O multiplexing

SYNOPSIS
       /* According to POSIX.1-2001, POSIX.1-2008 */
       #include <sys/select.h>

       /* According to earlier standards */
       #include <sys/time.h>
       #include <sys/types.h>
       #include <unistd.h>

       int select(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout);

       void FD_CLR(int fd, fd_set *set);
       int  FD_ISSET(int fd, fd_set *set);
       void FD_SET(int fd, fd_set *set);
       void FD_ZERO(fd_set *set);

       #include <sys/select.h>

       int pselect(int nfds, fd_set *readfds, fd_set *writefds,
                   fd_set *exceptfds, const struct timespec *timeout,
                   const sigset_t *sigmask);

DESCRIPTION
       select()  and pselect() allow a program to monitor multiple file descriptors,
       waiting until one or more of the file descriptors become "ready" for some class
       of I/O operation (e.g., input possible).  A file descriptor is considered ready
       if it is possible to perform a corresponding  I/O  operation  (e.g.,  read(2)
       without blocking, or a sufficiently small write(2)).

       Three independent sets of file descriptors are watched.  The file descriptors
       listed in readfds will be watched to see if  characters  become  available  for
       reading  (more  precisely, to see if a read will not block; in particular, a file
       descriptor is also ready on end-of-file).  The file descriptors in writefds will
       be watched to see if space is available for write (though a large write may still block).
       The file descriptors in exceptfds will be watched for  exceptional conditions.
       (For examples of some exceptional conditions, see the discussion of POLLPRI in poll(2).)

       On exit, each of the file descriptor sets is modified in place to indicate which file
       descriptors actually changed status.  (Thus, if using select() within a loop, the sets
       must be reinitialized before each call.)

RETURN VALUE
       On success, select() and pselect() return the number of file descriptors contained in
       the three returned descriptor sets (that is, the total number of bits that are set in
       readfds, writefds, exceptfds) which may be zero if the timeout expires before anything
       interesting happens.  On error, -1 is returned, and errno is set to indicate the error;
       the file descriptor sets are unmodified, and timeout becomes undefined.

EXAMPLE
       int main(void)
       {
           fd_set rfds;
           struct timeval tv;
           int retval;

           /* Watch stdin (fd 0) to see when it has input. */

           FD_ZERO(&rfds);
           FD_SET(0, &rfds);

           /* Wait up to five seconds. */

           tv.tv_sec = 5;
           tv.tv_usec = 0;

           retval = select(1, &rfds, NULL, NULL, &tv);
           /* Don't rely on the value of tv now! */

           if (retval == -1)
               perror("select()");
           else if (retval)
               printf("Data is available now.\n");
               /* FD_ISSET(0, &rfds) will be true. */
           else
               printf("No data within five seconds.\n");

           exit(EXIT_SUCCESS);
       }
Linux                    2017-09-15                                              SELECT(2)
```

Hence, the first 31 lines of the trace show the bash shell reacting to me typing
"./a.out". This can be easily understood by observing a few lines of the trace.

```txt
# cat shell_trace.txt | head -4
pselect6(1, [0], NULL, NULL, NULL, {[], 8}) = 1 (in [0])		// stdin is ready to read
read(0, ".", 1)                         = 1                     // reads '.'
select(1, [0], NULL, [0], {tv_sec=0, tv_usec=0}) = 0 (Timeout)
write(2, ".", 1)                        = 1                     // writes '.' to stdout
```

The C++ comments make it clear what each system call in the trace correspond to,
with the only exception being the select call that is waiting for any signals
received on stdin. Now lets look at the next 12 lines.

```txt
# cat shell_trace.txt | head -46 | tail -13
ioctl(0, SNDCTL_TMR_STOP or TCSETSW, {B38400 opost isig icanon echo ...}) = 0
rt_sigaction(SIGINT, {sa_handler=0x5584c8f03e60, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f5a150, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGTERM, {sa_handler=0x5584c8f03940, sa_mask=[], sa_flags=SA_RESTORER|SA_RESTART, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f5a150, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGHUP, {sa_handler=0x5584c8f04230, sa_mask=[HUP INT ILL TRAP ABRT BUS FPE USR1 SEGV USR2 PIPE ALRM TERM XCPU XFSZ VTALRM SYS], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f5a150, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGALRM, {sa_handler=0x5584c8f04230, sa_mask=[HUP INT ILL TRAP ABRT BUS FPE USR1 SEGV USR2 PIPE ALRM TERM XCPU XFSZ VTALRM SYS], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f5a150, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGWINCH, {sa_handler=0x5584c8f03930, sa_mask=[], sa_flags=SA_RESTORER|SA_RESTART, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f595c0, sa_mask=[], sa_flags=SA_RESTORER|SA_RESTART, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGINT, {sa_handler=0x5584c8f03e60, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f03e60, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigprocmask(SIG_BLOCK, [INT CHLD], [], 8) = 0
rt_sigprocmask(SIG_BLOCK, [CHLD], [INT CHLD], 8) = 0
rt_sigprocmask(SIG_SETMASK, [INT CHLD], NULL, 8) = 0
pipe([4, 5])                            = 0
clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD, child_tidptr=0x7f3a7242ca10) = 5603
setpgid(5603, 5603)                     = 0
```

This looks like a lot but it really only consists of a few functions that can
be analyzed one at a time: _ioctl_, *rt_sigaction*, *rt_sigprocmask*, _pipe_,
and _clone_. Let's look at _ioctl_.

```txt
IOCTL(2)                 Linux Programmer's Manual                               IOCTL(2)

NAME
       ioctl - control device

SYNOPSIS
       #include <sys/ioctl.h>

       int ioctl(int fd, unsigned long request, ...);

DESCRIPTION
       The ioctl() system call manipulates the underlying device parameters of special
       files. In particular, many operating characteristics of character special files
       (e.g., terminals) may be controlled with ioctl() requests. The argument fd must
       be an open file descriptor.

       The second argument is a device-dependent request code. The third argument is
       an untyped pointer to memory. It's traditionally char *argp (from the days before
       void * was valid C), and will be so named for this discussion.

       An ioctl() request has encoded in it whether the argument is an in parameter or out
       parameter, and the size of the argument argp in bytes.  Macros and defines used
       in specifying an ioctl() request are located in the file <sys/ioctl.h>.

RETURN VALUE
       Usually, on success zero is returned. A few ioctl() requests use the return value
       as an output parameter and return a nonnegative value on success. On error, -1 is
       returned, and errno is set appropriately.

ERRORS
       EBADF  fd is not a valid file descriptor.

       EFAULT argp references an inaccessible memory area.

       EINVAL request or argp is not valid.

       ENOTTY fd is not associated with a character special device.

       ENOTTY The specified request does not apply to the kind of object that the file
              descriptor fd references.

COLOPHON
       This page is part of release 4.16 of the Linux man-pages project. A description
       of the project, information about reporting bugs, and the latest version of
       this page, can be found at https://www.kernel.org/doc/man-pages/.

Linux                    2017-05-03                                              IOCTL(2)
```


