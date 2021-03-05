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

_pselect6_ is a system call that is used to multiplex I/O for processes,
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
# cat shell_trace.txt | head -46 | tail -12
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

Linux                    2017-05-03                                              IOCTL(2)
```

Using this man page along with the webpage below, we discover that the call to
_ioctl_ modifies the terminal settings before the shell begins to process the
`./a.out` command.

Link: https://unix.stackexchange.com/questions/242966/how-does-ssh-always-manage-to-reset-the-terminal-attributes

Now onto *rt_sigaction* and *rt_sigprocmask*!

```txt
SIGACTION(2)             Linux Programmer's Manual                               SIGACTION(2)

NAME
       sigaction, rt_sigaction - examine and change a signal action

SYNOPSIS
       #include <signal.h>

       int sigaction(int signum, const struct sigaction *act,
                     struct sigaction *oldact);

   Feature Test Macro Requirements for glibc (see feature_test_macros(7)):

       sigaction(): _POSIX_C_SOURCE

       siginfo_t: _POSIX_C_SOURCE >= 199309L

DESCRIPTION
       The sigaction() system call is used to change the action taken by a process on
       receipt of a specific signal. (See signal(7) for an overview of signals.)

       signum specifies the signal and can be any valid signal except SIGKILL and SIGSTOP.

       If act is non-NULL, the new action for signal signum is installed from act.
       If oldact is non-NULL, the previous action is saved in oldact.

RETURN VALUE
       sigaction() returns 0 on success; on error, -1 is returned, and errno is set
       to indicate the error.

ERRORS
       EFAULT act or oldact points to memory which is not a valid part of the process address space.

       EINVAL An  invalid  signal  was  specified.   This  will also be generated if an attempt is made to change the action for SIGKILL or SIGSTOP, which cannot be
              caught or ignored.

CONFORMING TO
       POSIX.1-2001, POSIX.1-2008, SVr4.

Linux                    2017-09-15                                              SIGACTION(2)
```

Upon reading this, we can clearly see that the following commands are modifying
the bash shell's signal handlers for SIGINT, SIGTERM, SIGHUP, SIGWINCH, and
SIGINT. This makes perfect sense; we have no idea how bash responds to these
signals and it is extremely unlikely that we ever want our programs to be ran
with the bash shell's signal handlers!

```txt
rt_sigaction(SIGINT, {sa_handler=0x5584c8f03e60, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f5a150, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGTERM, {sa_handler=0x5584c8f03940, sa_mask=[], sa_flags=SA_RESTORER|SA_RESTART, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f5a150, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGHUP, {sa_handler=0x5584c8f04230, sa_mask=[HUP INT ILL TRAP ABRT BUS FPE USR1 SEGV USR2 PIPE ALRM TERM XCPU XFSZ VTALRM SYS], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f5a150, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGALRM, {sa_handler=0x5584c8f04230, sa_mask=[HUP INT ILL TRAP ABRT BUS FPE USR1 SEGV USR2 PIPE ALRM TERM XCPU XFSZ VTALRM SYS], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f5a150, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGWINCH, {sa_handler=0x5584c8f03930, sa_mask=[], sa_flags=SA_RESTORER|SA_RESTART, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f595c0, sa_mask=[], sa_flags=SA_RESTORER|SA_RESTART, sa_restorer=0x7f3a72466ec0}, 8) = 0
rt_sigaction(SIGINT, {sa_handler=0x5584c8f03e60, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, {sa_handler=0x5584c8f03e60, sa_mask=[], sa_flags=SA_RESTORER, sa_restorer=0x7f3a72466ec0}, 8) = 0
```

Now we can move onto *rt_sigprocmask*, which is likely to be straightforward.

```txt
SIGPROCMASK(2)           Linux Programmer's Manual                               SIGPROCMASK(2)

NAME
       sigprocmask, rt_sigprocmask - examine and change blocked signals

SYNOPSIS
       #include <signal.h>

       /* Prototype for the glibc wrapper function */
       int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);

       /* Prototype for the underlying system call */
       int rt_sigprocmask(int how, const kernel_sigset_t *set,
                          kernel_sigset_t *oldset, size_t sigsetsize);

       /* Prototype for the legacy system call (deprecated) */
       int sigprocmask(int how, const old_kernel_sigset_t *set,
                       old_kernel_sigset_t *oldset);

   Feature Test Macro Requirements for glibc (see feature_test_macros(7)):

       sigprocmask(): _POSIX_C_SOURCE

DESCRIPTION
       sigprocmask() is used to fetch and/or change the signal mask of the calling
       thread. The signal mask is the set of signals whose delivery is currently
       blocked for the caller (see also signal(7) for more details).

       The behavior of the call is dependent on the value of how, as follows.

       SIG_BLOCK
              The set of blocked signals is the union of the current set and the set argument.

       SIG_UNBLOCK
              The signals in set are removed from the current set of blocked signals. It is
              permissible to attempt to unblock a signal which is not blocked.

       SIG_SETMASK
              The set of blocked signals is set to the argument set.

       If oldset is non-NULL, the previous value of the signal mask is stored in oldset.

       If set is NULL, then the signal mask is unchanged (i.e., how is ignored), but the current
       value of the signal mask is nevertheless returned in oldset (if it is not NULL).

       A set of functions for modifying and inspecting variables of type sigset_t ("signal sets")
       is described in sigsetops(3).

       The use of sigprocmask() is unspecified in a multithreaded process; see pthread_sigmask(3).

RETURN VALUE
       sigprocmask() returns 0 on success and -1 on error.  In the event of an error, errno is set to indicate the cause.

Linux                    2017-09-15                                              SIGPROCMASK(2)
```

While it is unclear why three system calls were used for it, the signal mask is
set to block (SIGCHLD | SIGINT) and to store the old signal mask in an unnamed
kernel\_sigset\_t structure denoted by '[]'.

1. SIGCHLD - Child stopped or terminated
2. SIGINT  - Interrupt from keyboard

```txt
rt_sigprocmask(SIG_BLOCK, [INT CHLD], [], 8) = 0
rt_sigprocmask(SIG_BLOCK, [CHLD], [INT CHLD], 8) = 0
rt_sigprocmask(SIG_SETMASK, [INT CHLD], NULL, 8) = 0
```

For the _pipe_ call, it is important to stress that pipes are unidirectional
in Linux. Other than that, it is self-explanatory.

```txt
PIPE(2)                  Linux Programmer's Manual                               PIPE(2)

NAME
       pipe, pipe2 - create pipe

SYNOPSIS
       #include <unistd.h>

       int pipe(int pipefd[2]);

       #define _GNU_SOURCE             /* See feature_test_macros(7) */
       #include <fcntl.h>              /* Obtain O_* constant definitions */
       #include <unistd.h>

       int pipe2(int pipefd[2], int flags);

DESCRIPTION
       pipe() creates a pipe, a unidirectional data channel that can be used for
       interprocess communication. The array pipefd is used to return two file descrip‐
       tors referring to the ends of the pipe. pipefd[0] refers to the read end of the
       pipe. pipefd[1] refers to the write end of the pipe. Data written  to  the write
       end of the pipe is buffered by the kernel until it is read from the read end of
       the pipe. For further details, see pipe(7).

RETURN VALUE
       On success, zero is returned.  On error, -1 is returned, and errno is set appropriately.

       On Linux (and other systems), pipe() does not modify pipefd on failure.
       A requirement standardizing this behavior was added in POSIX.1-2016.
       The Linux-specific pipe2() system call likewise does not modify pipefd
       on failure.

Linux                    2017-11-26                                              PIPE(2)
```

Now that all of the previous functions have been explored, we finally get to
the function call that spawns a new process that will be execve'd into our
program.

```txt
CLONE(2)                 Linux Programmer's Manual                               CLONE(2)

NAME
       clone, __clone2 - create a child process

SYNOPSIS
       /* Prototype for the glibc wrapper function */

       #define _GNU_SOURCE
       #include <sched.h>

       int clone(int (*fn)(void *), void *child_stack,
                 int flags, void *arg, ...
                 /* pid_t *ptid, void *newtls, pid_t *ctid */ );

       /* For the prototype of the raw system call, see NOTES */

DESCRIPTION
       clone() creates a new process, in a manner similar to fork(2).

       This page describes both the glibc clone() wrapper function and the underlying
       system call on which it is based.  The main text describes the wrapper function;
       the differences for the raw system call are described toward the end of this page.

       Unlike fork(2), clone() allows the child process to share parts of its execution
       context with the calling process, such as the virtual address space, the table of
       file descriptors, and the table of signal handlers. (Note that on this manual page,
       "calling process" normally corresponds to "parent process". But see the description
       of CLONE_PARENT below.)

       One use of clone() is to implement threads: multiple flows of control in a program
       that run concurrently in a shared address space.

       When the child process is created with clone(), it commences execution by calling
       the function pointed to by the argument fn. (This  differs  from  fork(2), where
       execution continues in the child from the point of the fork(2) call.)
       The arg argument is passed as the argument of the function fn.

       When the fn(arg) function returns, the child process terminates. The integer returned
       by fn is the exit status for the child process. The child process may also terminate
       explicitly by calling exit(2) or after receiving a fatal signal.

       The child_stack argument specifies the location of the stack used by the child process.
       Since the child and calling process may share memory, it is not possible  for the child
       process to execute in the same stack as the calling process.  The calling process must
       therefore set up memory space for the child stack and pass a pointer to this space to
       clone(). Stacks grow downward on all processors that run Linux (except the HP PA
       processors), so child_stack usually points to the topmost address of the memory space
       set up for the child stack.

       The low byte of flags contains the number of the termination signal sent to the parent
       when the child dies. If this signal is specified as anything other than SIGCHLD, then
       the parent process must specify the __WALL or __WCLONE options when waiting for the child
       with wait(2). If no signal is  specified, then the parent process is not signaled when the
       child terminates.

       flags may also be bitwise-ORed with zero or more of the following constants, in order to
       specify what is shared between the calling process and the child process:

       CLONE_CHILD_CLEARTID (since Linux 2.5.49)
              Clear (zero) the child thread ID at the location ctid in child memory when the child exits,
              and do a wakeup on the futex at that address. The address involved may be changed by the
              set_tid_address(2) system call. This is used by threading libraries.

       CLONE_CHILD_SETTID (since Linux 2.5.49)
              Store the child thread ID at the location ctid in the child's memory. The store operation
              completes before clone() returns control to user space.

RETURN VALUE
       On success, the thread ID of the child process is returned in the caller's thread of execution.
       On failure, -1 is returned in the caller's context, no child process will be created, and errno
       will be set appropriately.

Linux                    2017-09-15                                              CLONE(2)
```

From the above man page, we can determine that the following command from the
trace file clones a child process with no stack that will execute the command
`./a.out` and return its tid 5603. The details of the flags, the child\_tidptr,
and the missing fn argument from the strace will be discussed in more detail in
a later set of notes when the internals of the _clone()_ system call are analyzed.

```txt
clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD, child_tidptr=0x7f3a7242ca10) = 5603
```

For now though, we have traced every system call of the shell command `./a.out` up to
creation of a new process that will be execve'd with our program. We will continue
tracing the execution of `./a.out` in the next set of notes.
