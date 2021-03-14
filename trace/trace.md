# Linux Kernel Internals Simplified: Linux Hacking Tools 

The first tool introduced in this book is _ftrace_, which is a built-in
debugging tool for Linux.

Using _ftrace_ is fairly simple. To create a function trace of all the
calls made by the kernel in a given period of time we do the following:

```txt
# sudo -i
# cd /sys/kernel/debug/tracing
# echo function > current_tracer
# echo 1 > tracing_on
# Wait for 5 seconds ^C
# echo 0 > tracing_on
# cat trace | head -30
# tracer: function
#
# entries-in-buffer/entries-written: 204964/18371579   #P:4
#
#                                _-----=> irqs-off
#                               / _----=> need-resched
#                              | / _---=> hardirq/softirq
#                              || / _--=> preempt-depth
#                              ||| /     delay
#           TASK-PID     CPU#  ||||   TIMESTAMP  FUNCTION
#              | |         |   ||||      |         |
            Xorg-1407    [001] d... 139685.630102: record_times <-psi_group_change
            Xorg-1407    [001] d... 139685.630102: psi_group_change <-psi_task_change
            Xorg-1407    [001] d... 139685.630102: record_times <-psi_group_change
            Xorg-1407    [001] d... 139685.630103: psi_group_change <-psi_task_change
            Xorg-1407    [001] d... 139685.630103: record_times <-psi_group_change
            Xorg-1407    [001] d... 139685.630103: psi_group_change <-psi_task_change
            Xorg-1407    [001] d... 139685.630103: record_times <-psi_group_change
            Xorg-1407    [001] d... 139685.630104: dequeue_task_fair <-__schedule
            Xorg-1407    [001] d... 139685.630104: dequeue_entity <-dequeue_task_fair
            Xorg-1407    [001] d... 139685.630104: update_curr <-dequeue_entity
            Xorg-1407    [001] d... 139685.630104: update_min_vruntime <-update_curr
            Xorg-1407    [001] d... 139685.630105: cpuacct_charge <-update_curr
            Xorg-1407    [001] d... 139685.630105: __cgroup_account_cputime <-update_curr
            Xorg-1407    [001] d... 139685.630105: cgroup_rstat_updated <-__cgroup_account_cputime
            Xorg-1407    [001] d... 139685.630105: __update_load_avg_se <-update_load_avg
            Xorg-1407    [001] d... 139685.630105: __update_load_avg_cfs_rq <-update_load_avg
            Xorg-1407    [001] d... 139685.630106: clear_buddies <-dequeue_entity
            Xorg-1407    [001] d... 139685.630106: account_entity_dequeue <-dequeue_entity
            Xorg-1407    [001] d... 139685.630106: update_cfs_group <-dequeue_entity
```

As we can see above, a trace file is automatically created at /sys/kernel/tracing/trace.
Using _grep_, we can search for certain kinds of calls like interrupt handlers.

```txt
# cat trace | grep irq_enter | head -20
          <idle>-0       [001] d... 139685.632569: irq_enter_rcu <-sysvec_apic_timer_interrupt
          <idle>-0       [001] d... 139685.632569: tick_irq_enter <-irq_enter_rcu
          <idle>-0       [001] d... 139685.632570: tick_check_oneshot_broadcast_this_cpu <-tick_irq_enter
          <idle>-0       [001] d... 139685.632570: ktime_get <-tick_irq_enter
          <idle>-0       [001] d... 139685.632570: update_ts_time_stats <-tick_irq_enter
          <idle>-0       [001] d... 139685.632571: _local_bh_enable <-irq_enter_rcu
          <idle>-0       [001] d.s. 139685.632594: irq_enter_rcu <-sysvec_irq_work
          <idle>-0       [001] d... 139685.633628: irq_enter_rcu <-common_interrupt
          <idle>-0       [001] d... 139685.633629: tick_irq_enter <-irq_enter_rcu
          <idle>-0       [001] d... 139685.633629: tick_check_oneshot_broadcast_this_cpu <-tick_irq_enter
          <idle>-0       [001] d... 139685.633629: ktime_get <-tick_irq_enter
          <idle>-0       [001] d... 139685.633629: update_ts_time_stats <-tick_irq_enter
          <idle>-0       [001] d... 139685.633642: _local_bh_enable <-irq_enter_rcu
          <idle>-0       [001] d... 139685.636586: irq_enter_rcu <-sysvec_apic_timer_interrupt
          <idle>-0       [001] d... 139685.636587: tick_irq_enter <-irq_enter_rcu
          <idle>-0       [001] d... 139685.636587: tick_check_oneshot_broadcast_this_cpu <-tick_irq_enter
          <idle>-0       [001] d... 139685.636587: ktime_get <-tick_irq_enter
          <idle>-0       [001] d... 139685.636599: update_ts_time_stats <-tick_irq_enter
          <idle>-0       [001] d... 139685.636599: _local_bh_enable <-irq_enter_rcu
          <idle>-0       [001] d... 139685.640635: irq_enter_rcu <-sysvec_apic_timer_interrupt
```

While the function trace is certainly useful, we can use _ftrace_ to trace a
particular function call using function\_graph. In this case, we will trace
\_do\_fork just like the book since the kernel is constantly forking processes.

```txt
# echo function_graph > current_tracer
# echo _do_fork > set_graph_function
# echo 10 > max_graph_depth
# echo 1 > tracing_on && /tmp/a.out && echo 0 > tracing_on
# cat trace | head -40
# tracer: function_graph
#
# CPU  DURATION                  FUNCTION CALLS
# |     |   |                     |   |   |   |
 2)               |  _do_fork() {
 2)               |    copy_process() {
 2)   0.641 us    |      task_active_pid_ns();
 2)   0.630 us    |      _raw_spin_lock_irq();
 2)               |      recalc_sigpending() {
 2)   0.499 us    |        recalc_sigpending_tsk();
 2)   1.403 us    |      }
 2)               |      dup_task_struct() {
 2)   0.522 us    |        tsk_fork_get_node();
 2)               |        kmem_cache_alloc_node() {
 2)               |          _cond_resched() {
 2)   0.379 us    |            rcu_all_qs();
 2)   1.148 us    |          }
 2)   0.387 us    |          should_failslab();
 2)   1.258 us    |          memcg_kmem_get_cache();
 2)   0.550 us    |          memcg_kmem_put_cache();
 2)   8.018 us    |        }
 2)               |        __memcg_kmem_charge_page() {
 2)   1.137 us    |          get_mem_cgroup_from_mm();
 2)               |          __memcg_kmem_charge() {
 2)   0.677 us    |            try_charge();
 2)               |            page_counter_try_charge() {
 2)   0.511 us    |              propagate_protected_usage();
 2)   0.368 us    |              propagate_protected_usage();
 2)   0.455 us    |              propagate_protected_usage();
 2)   0.397 us    |              propagate_protected_usage();
 2)   4.749 us    |            }
 2)   6.717 us    |          }
 2)   9.655 us    |        }
 2)   0.569 us    |        __mod_memcg_state();
 2)               |        __memcg_kmem_charge_page() {
 2)   0.412 us    |          get_mem_cgroup_from_mm();
 2)               |          __memcg_kmem_charge() {
 2)   0.514 us    |            try_charge();
 2)               |            page_counter_try_charge() {
 2)   0.383 us    |              propagate_protected_usage();
```

The function\_graph tracer creates a very useful visualization of the code flow
that saves the enormous amount of time that would be spent if one were to read
through source code via a cross-referencer.

Let's look at another example of a function we will be analyzing in another set
of notes.

```txt
# echo ksys_write > set_graph_function
# echo 5 > max_graph_depth
# echo 1 > tracing_on && /tmp/a.out && echo 0 > tracing_on
# cat trace | head -40

# tracer: function_graph
#
# CPU  DURATION                  FUNCTION CALLS
# |     |   |                     |   |   |   |
 3)               |        security_file_permission() {
 3)   1.174 us    |          apparmor_file_permission();
 3)   3.067 us    |        }
 3)   4.116 us    |      } /* rw_verify_area */
 3)               |      eventfd_write() {
 3)   0.639 us    |        _raw_spin_lock_irq();
 3)               |        __wake_up_locked_key() {
 3) + 16.711 us   |          __wake_up_common();
 3) + 17.694 us   |        }
 3) + 20.066 us   |      }
 3)   0.716 us    |      fsnotify_parent();
 3)   0.538 us    |      fsnotify();
 3) + 28.148 us   |    } /* vfs_write */
 3)               |    fput() {
 3)   0.547 us    |      fput_many();
 3)   1.604 us    |    }
 3) + 34.720 us   |  } /* ksys_write */
 3)               |  ksys_write() {
 3)               |    __fdget_pos() {
 3)               |      __fget_light() {
 3)   0.409 us    |        __fget_files();
 3)   1.105 us    |      }
 3)   1.767 us    |    }
 3)               |    vfs_write() {
 3)               |      rw_verify_area() {
 3)               |        security_file_permission() {
 3)   0.704 us    |          apparmor_file_permission();
 3)   1.457 us    |        }
 3)   2.093 us    |      }
 3)               |      eventfd_write() {
 3)   0.363 us    |        _raw_spin_lock_irq();
 3)               |        __wake_up_locked_key() {
 3)   9.362 us    |          __wake_up_common();
 3) + 10.025 us   |        }
 3) + 11.532 us   |      }
 3)   0.467 us    |      fsnotify_parent();
```

Since ksys\_write is such a frequently called function, the beginning of the
trace shows code at the end of ksys\_write's execution. Hence, a better view
of the code flow would be the following:

```txt
 ------------------------------------------
 1)  InputTh-1430  =>  a.out-20727  
 ------------------------------------------

 1)               |  ksys_write() {
 1)               |    __fdget_pos() {
 1)   0.319 us    |      __fget_light();
 1)   1.176 us    |    }
 1)               |    vfs_write() {
 1)               |      rw_verify_area() {
 1)               |        security_file_permission() {
 1)   0.516 us    |          apparmor_file_permission();
 1)   1.039 us    |        }
 1)   1.425 us    |      }
 1)               |      tty_write() {
 1)   0.302 us    |        tty_paranoia_check();
 1)               |        tty_ldisc_ref_wait() {
 1)   0.338 us    |          ldsem_down_read();
 1)   0.820 us    |        }
 1)               |        tty_write_lock() {
 1)   0.224 us    |          mutex_trylock();
 1)   0.692 us    |        }
 1)               |        __check_object_size() {
 1)   0.184 us    |          check_stack_object();
 1)   0.232 us    |          __virt_addr_valid();
 1)   0.287 us    |          __check_heap_object();
 1)   1.607 us    |        }
 1)   0.197 us    |        down_read_trylock();
 1)               |        _cond_resched() {
 1)   0.187 us    |          rcu_all_qs();
 1)   0.553 us    |        }
 1)               |        find_vma() {
 1)   0.268 us    |          vmacache_find();
 1)   0.190 us    |          vmacache_update();
 1)   1.096 us    |        }
 1)               |        handle_mm_fault() {
 1)   0.190 us    |          mem_cgroup_from_task();
 1)   0.190 us    |          __count_memcg_events();
 1)   1.455 us    |          __handle_mm_fault();
 1)   2.720 us    |        }
 1)   0.204 us    |        up_read();
 1)               |        n_tty_write() {
 1)   0.331 us    |          down_read();
 1)   0.516 us    |          process_echoes();
 1)   0.339 us    |          add_wait_queue();
 1)   0.195 us    |          tty_hung_up_p();
 1)   0.327 us    |          mutex_lock();
 1)   0.505 us    |          tty_write_room();
 1)   8.180 us    |          pty_write();
 1)   0.236 us    |          mutex_unlock();
 1)   0.321 us    |          mutex_lock();
 1)   0.413 us    |          tty_write_room();
 1)   0.902 us    |          do_output_char();
 1)   0.202 us    |          mutex_unlock();
 1)   0.338 us    |          remove_wait_queue();
 1)   0.201 us    |          up_read();
 1) + 16.808 us   |        }
 1)   0.206 us    |        ktime_get_real_seconds();
 1)               |        tty_write_unlock() {
 1)   0.246 us    |          mutex_unlock();
 1)   0.501 us    |          __wake_up();
 1)   1.338 us    |        }
 1)               |        tty_ldisc_deref() {
 1)   0.199 us    |          ldsem_up_read();
 1)   0.882 us    |        }
 1) + 31.389 us   |      }
 1)   0.339 us    |      fsnotify_parent();
 1)   0.347 us    |      fsnotify();
 1) + 34.809 us   |    }
 1) + 37.629 us   |  }
 3)               |  ksys_write() {
 3)               |    __fdget_pos() {
 3)               |      __fget_light() {
 3)   0.356 us    |        __fget_files();
 3)   1.147 us    |      }
 3)   1.874 us    |    }
 3)               |    vfs_write() {
 3)               |      rw_verify_area() {
 3)               |        security_file_permission() {
 3)   0.809 us    |          apparmor_file_permission();
 3)   1.570 us    |        }
 3)   2.339 us    |      }
 3)               |      eventfd_write() {
 3)   0.323 us    |        _raw_spin_lock_irq();
 3)   1.198 us    |      }
 3)   0.431 us    |      fsnotify_parent();
 3)   0.386 us    |      fsnotify();
 3)   6.129 us    |    }
 3)               |    fput() {
 3)   0.348 us    |      fput_many();
 3)   1.012 us    |    }
 3) + 11.032 us   |  }
 3)               |  ksys_write() {
 3)               |    __fdget_pos() {
 3)               |      __fget_light() {
 3)   0.318 us    |        __fget_files();
 3)   0.996 us    |      }
 3)   1.653 us    |    }
 3)               |    vfs_write() {
 3)               |      rw_verify_area() {
 3)               |        security_file_permission() {
 3)   0.566 us    |          apparmor_file_permission();
 3)   1.239 us    |        }
 3)   1.881 us    |      }
 3)               |      eventfd_write() {
 3)   0.336 us    |        _raw_spin_lock_irq();
 3)   1.036 us    |      }
 3)   0.326 us    |      fsnotify_parent();
 3)   0.344 us    |      fsnotify();
 3)   5.275 us    |    }
 3)               |    fput() {
 3)   0.323 us    |      fput_many();
 3)   0.959 us    |    }
 3)   9.240 us    |  }
```

Hence, we can see that ksys\_write, the function that is called when we do a
write system call, calls tty\_write after obtaining the offset with \_\_fdget\_pos.


Although it is not shown in the code blocks, the biggest issue with using _ftrace_
as it is described in the book is that it does not trace a particular PID.
Thankfully, this can be be solved using shell scripts as described at elinux.org/Ftrace.

```bash
#!/bin/sh

# Usage: ./trace_command `pwd`/<executable>

FTRACE="/sys/kernel/tracing"

echo $$ > $FTRACE/set_ftrace_pid
echo function_graph > $FTRACE/current_tracer
echo 5 > $FTRACE/max_graph_depth
echo ksys_write > $FTRACE/set_graph_function
cat /dev/null > $FTRACE/trace
echo 1 > $FTRACE/tracing_on
exec $*
```

Finally, it is worth showing how to clean up the trace file after using _ftrace_.

```txt
# echo nop > current_tracer
# cat /dev/null > trace
```
