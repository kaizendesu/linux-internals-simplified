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
       int select(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout);

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
       int sigaction(int signum, const struct sigaction *act,
                     struct sigaction *oldact);

DESCRIPTION
       The sigaction() system call is used to change the action taken by a process on
       receipt of a specific signal. (See signal(7) for an overview of signals.)

       signum specifies the signal and can be any valid signal except SIGKILL and SIGSTOP.

       If act is non-NULL, the new action for signal signum is installed from act.
       If oldact is non-NULL, the previous action is saved in oldact.

RETURN VALUE
       sigaction() returns 0 on success; on error, -1 is returned, and errno is set
       to indicate the error.

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
       /* Prototype for the glibc wrapper function */
       int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);

       /* Prototype for the underlying system call */
       int rt_sigprocmask(int how, const kernel_sigset_t *set,
                          kernel_sigset_t *oldset, size_t sigsetsize);

       /* Prototype for the legacy system call (deprecated) */
       int sigprocmask(int how, const old_kernel_sigset_t *set,
                       old_kernel_sigset_t *oldset);

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
       int pipe(int pipefd[2]);

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

Now that all of the previous functions have been explored, we have reached the
_clone()_ system call.

```txt
CLONE(2)                 Linux Programmer's Manual                               CLONE(2)

NAME
       clone, __clone2 - create a child process

SYNOPSIS
       int clone(int (*fn)(void *), void *child_stack,
                 int flags, void *arg, ...
                 /* pid_t *ptid, void *newtls, pid_t *ctid */ );

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

From the above man page, we determine that the _clone_ system call below creates
a child process with no stack and returns its pid/tid 5603. The details of the flags,
the child\_tidptr, and the missing fn argument from the strace will be discussed in
more detail in a later set of notes when the internals of the _clone()_ system call
are analyzed.

```txt
clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD, child_tidptr=0x7f3a7242ca10) = 5603
```

For now though, let us examine the last set of system calls before the bash shell
waits for the newly created child process.

```txt
# cat shell_trace.txt | head -52 | tail -9
setpgid(5603, 5603)                     = 0
rt_sigprocmask(SIG_SETMASK, [], NULL, 8) = 0
rt_sigprocmask(SIG_BLOCK, [CHLD], [], 8) = 0
close(4)                                = 0
close(5)                                = 0
ioctl(255, TIOCGPGRP, [5603])           = 0
rt_sigprocmask(SIG_SETMASK, [], NULL, 8) = 0
rt_sigprocmask(SIG_BLOCK, [CHLD], [], 8) = 0
wait4(-1, [{WIFEXITED(s) && WEXITSTATUS(s) == 23}], WSTOPPED|WCONTINUED, NULL) = 5603
```

Of these 9 system calls, we are only interested in two of them, the first one
being _setpgid_. This system call sets the pgid of the child process to 5603.
Basically, we use this call to set the child process's pgid equal to its pid.

The second system call we are interested in, and the last one we execute
before we switch to the child process's point-of-view, is _wait4_. This is
a specialized system call, so it will need two man pages to fully understand it.

```txt
WAIT4(2)                 Linux Programmer's Manual                               WAIT4(2)

NAME
       wait3, wait4 - wait for process to change state, BSD style

SYNOPSIS
       pid_t wait3(int *wstatus, int options,
                   struct rusage *rusage);

       pid_t wait4(pid_t pid, int *wstatus, int options,
                   struct rusage *rusage);

DESCRIPTION
       These functions are nonstandard; in new programs, the use of waitpid(2) or waitid(2) is preferable.

       The  wait3()  and wait4() system calls are similar to waitpid(2), but additionally return resource usage information about the child in the structure pointed
       to by rusage.

       Other than the use of the rusage argument, the following wait3() call:

           wait3(wstatus, options, rusage);

       is equivalent to:

           waitpid(-1, wstatus, options);

       Similarly, the following wait4() call:

           wait4(pid, wstatus, options, rusage);

       is equivalent to:

           waitpid(pid, wstatus, options);

       In other words, wait3() waits of any child, while wait4() can be used to select a specific child, or children, on which to wait.   See  wait(2)  for  further
       details.

       If rusage is not NULL, the struct rusage to which it points will be filled with accounting information about the child.  See getrusage(2) for details.

RETURN VALUE
       As for waitpid(2).

Linux                    2018-04-30                                              WAIT4(2)

WAIT(2)                  Linux Programmer's Manual                               WAIT(2)

NAME
       wait, waitpid, waitid - wait for process to change state

SYNOPSIS
       #include <sys/types.h>
       #include <sys/wait.h>

       pid_t wait(int *wstatus);

       pid_t waitpid(pid_t pid, int *wstatus, int options);

       int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);
                       /* This is the glibc and POSIX interface; see
                          NOTES for information on the raw system call. */

   Feature Test Macro Requirements for glibc (see feature_test_macros(7)):

       waitid():
           Since glibc 2.26: _XOPEN_SOURCE >= 500 ||
               _POSIX_C_SOURCE >= 200809L
           Glibc 2.25 and earlier:
               _XOPEN_SOURCE
                   || /* Since glibc 2.12: */ _POSIX_C_SOURCE >= 200809L
                   || /* Glibc versions <= 2.19: */ _BSD_SOURCE

DESCRIPTION
       All of these system calls are used to wait for state changes in a child of the
       calling process, and obtain information about the child whose state has changed.
       A state change is considered to be: the child terminated; the child was stopped by
       a signal; or the child was resumed by a signal. In the case of a terminated child,
       performing a wait allows the system to release the resources associated with the
       child; if a wait is not performed, then the terminated child remains in a "zombie"
       state(see NOTES below).

       If a child has already changed state, then these calls return immediately. Otherwise,
       they block until either a child changes state or a signal handler interrupts the
       call (assuming that system calls are not automatically restarted using the SA_RESTART
       flag of sigaction(2)). In the remainder of this page, a child whose state has changed
       and which has not yet been waited upon by one of these system calls is termed waitable.

   wait() and waitpid()
       The wait() system call suspends execution of the calling thread until one of its
       children terminates.  The call wait(&wstatus) is equivalent to:

           waitpid(-1, &wstatus, 0);

       The waitpid() system call suspends execution of the calling thread until a child
       specified by pid argument has changed state.  By  default,  waitpid()  waits only for
       terminated children, but this behavior is modifiable via the options argument, as described below.

       The value of pid can be:

       < -1   meaning wait for any child process whose process group ID is equal to the absolute value of pid.

       -1     meaning wait for any child process.

       0      meaning wait for any child process whose process group ID is equal to that of the calling process.

       > 0    meaning wait for the child whose process ID is equal to the value of pid.

       The value of options is an OR of zero or more of the following constants:

       WCONTINUED (since Linux 2.6.10)
                   also return if a stopped child has been resumed by delivery of SIGCONT.

       (For Linux-only options, see below.)

       If wstatus is not NULL, wait() and waitpid() store status information in the int to which it points. This
       integer can be inspected with the following macros (which take the integer itself as an argument, not
       a pointer to it, as is done in wait() and waitpid()!):

       WIFEXITED(wstatus)
              returns true if the child terminated normally, that is, by calling exit(3) or _exit(2), or by returning from main().

       WEXITSTATUS(wstatus)
              returns  the  exit  status  of  the child.  This consists of the least significant 8 bits of the status argument
              that the child specified in a call to exit(3) or _exit(2) or as the argument for a return statement in main().
              This macro should be employed only if WIFEXITED returned true.

   waitid()
       The waitid() system call (available since Linux 2.6.9) provides more precise control over which child state changes to wait for.

       The idtype and id arguments select the child(ren) to wait for, as follows:

       idtype == P_PID
              Wait for the child whose process ID matches id.

       idtype == P_PGID
              Wait for any child whose process group ID matches id.

       idtype == P_ALL
              Wait for any child; id is ignored.

       The child state changes to wait for are specified by ORing one or more of the following flags in options:

       WEXITED     Wait for children that have terminated.

       WSTOPPED    Wait for children that have been stopped by delivery of a signal.

       WCONTINUED  Wait for (previously stopped) children that have been resumed by delivery of SIGCONT.

       The following flags may additionally be ORed in options:

       WNOHANG     As for waitpid().

       WNOWAIT     Leave the child in a waitable state; a later wait call can be used to again retrieve the child status information.

       Upon successful return, waitid() fills in the following fields of the siginfo_t structure pointed to by infop:

       si_pid      The process ID of the child.

       si_uid      The real user ID of the child.  (This field is not set on most other implementations.)

       si_signo    Always set to SIGCHLD.

       si_status   Either the exit status of the child, as given to _exit(2) (or exit(3)), or the signal that caused the child to
                   terminate, stop, or continue. The si_code field can be used to determine how to interpret this field.

       si_code     Set  to  one  of:  CLD_EXITED (child called _exit(2)); CLD_KILLED (child killed by signal); CLD_DUMPED (child killed by signal, and dumped core);
                   CLD_STOPPED (child stopped by signal); CLD_TRAPPED (traced child has trapped); or CLD_CONTINUED (child continued by SIGCONT).

       If WNOHANG was specified in options and there were no children in a waitable state, then waitid() returns 0 immediately and the state of the siginfo_t struc‐
       ture  pointed  to  by infop depends on the implementation.  To (portably) distinguish this case from that where a child was in a waitable state, zero out the
       si_pid field before the call and check for a nonzero value in this field after the call returns.

       POSIX.1-2008 Technical Corrigendum 1 (2013) adds the requirement that when WNOHANG is specified in options and there were no children in  a  waitable  state,
       then waitid() should zero out the si_pid and si_signo fields of the structure.  On Linux and other implementations that adhere to this requirement, it is not
       necessary to zero out the si_pid field before calling waitid().  However, not all implementations follow the POSIX.1 specification on this point.

RETURN VALUE
       wait(): on success, returns the process ID of the terminated child; on error, -1 is returned.

       waitpid(): on success, returns the process ID of the child whose state has changed; if WNOHANG was specified and one or  more  child(ren)  specified  by  pid
       exist, but have not yet changed state, then 0 is returned.  On error, -1 is returned.

       waitid(): returns 0 on success or if WNOHANG was specified and no child(ren) specified by id has yet changed state; on error, -1 is returned.

       Each of these calls sets errno to an appropriate value in the case of an error.

Linux                    2018-04-30                                              WAIT(2)
```

Hence, bash calls _wait4_ to wait for the first child process that changes its state,
or in other words, the first child process to exit or be stopped by a signal. We
know for sure that this wait call stops blocking for child process we created with
_clone_ because it returns 5603 as the pid.

We have now traced through every system call of the shell command `./a.out` up 
to the creation of the new process and the bash shell suspending its execution.
We will continue tracing through `./a.out` from the child process's point-of-view
in the next set of notes.
