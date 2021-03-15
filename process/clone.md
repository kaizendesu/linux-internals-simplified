# Linux Kernel Internals Simplified: Running Processes in the Shell 3

This set of notes will be the first of three that explore the kernel source
code executed on behalf of our `a.out` program. We can briefly summarize this
source code as three system calls: _clone()_, _execve()_, and _write()_.

```c
#include <unistd.h>

/* The function we compile into a.out */
int		main(void)
{
	return syscall(1, 1, "Hello standard output!\n", 23);
}
```

Naturally, we will begin with the _clone()_ system call. Let's begin with a
complete _ftrace_ of the _clone()_ system call. First, we will open two tabs
in Terminal, one in the directory with the our `a.out` program and the other
inside the debugfs.

```txt
# ps -ef | grep bash
elliot     25584    1318  0 19:24 ?        00:00:00 /bin/bash /usr/bin/brave-browser-stable
elliot     26157   26149  0 20:09 pts/0    00:00:00 bash
elliot     27049   26149  0 20:50 pts/1    00:00:00 bash
root       27095   27094  0 20:53 pts/1    00:00:00 -bash
elliot     27174   27168  0 21:00 pts/0    00:00:00 /bin/bash -c ( ps -ef | grep bash)>/tmp/vOKPzov/0 2>&1
elliot     27175   27174  0 21:00 pts/0    00:00:00 /bin/bash -c ( ps -ef | grep bash)>/tmp/vOKPzov/0 2>&1
elliot     27177   27175  0 21:00 pts/0    00:00:00 grep bash

# sudo ./trace_pid_command 26157 _do_fork
# ./a.out
# sudo cp $DEBUG/trace trace
# cat trace
# tracer: function_graph
#
# CPU  DURATION                  FUNCTION CALLS
# |     |   |                     |   |   |   |
 3)   2.487 us    |  finish_task_switch();
 2)               |  _do_fork() {
 2)               |    copy_process() {
 2)   0.261 us    |      _raw_spin_lock_irq();
 2)               |      recalc_sigpending() {
 2)   0.215 us    |        recalc_sigpending_tsk();
 2)   0.655 us    |      }
 2)               |      dup_task_struct() {
 2)   0.240 us    |        tsk_fork_get_node();
 2)               |        kmem_cache_alloc_node() {
 2)   0.296 us    |          _cond_resched();
 2)   0.202 us    |          should_failslab();
 2)   0.551 us    |          memcg_kmem_get_cache();
 2)   0.291 us    |          memcg_kmem_put_cache();
 2)   3.243 us    |        }
 2)               |        __vmalloc_node_range() {
 2)   8.953 us    |          __get_vm_area_node();
 2)   0.788 us    |          __kmalloc_node();
 2)   2.359 us    |          alloc_pages_current();
 2)   0.288 us    |          _cond_resched();
 2)   1.552 us    |          alloc_pages_current();
 2)   0.269 us    |          _cond_resched();
 2)   1.530 us    |          alloc_pages_current();
 2)   0.275 us    |          _cond_resched();
 2)   1.603 us    |          alloc_pages_current();
 2)   0.270 us    |          _cond_resched();
 2)   0.559 us    |          map_kernel_range_noflush();
 2) + 21.454 us   |        }
 2)               |        find_vm_area() {
 2)   0.465 us    |          find_vmap_area();
 2)   0.924 us    |        }
 2)               |        __memcg_kmem_charge_page() {
 2)   0.467 us    |          get_mem_cgroup_from_mm();
 2)   1.503 us    |          __memcg_kmem_charge();
 2)   2.765 us    |        }
 2)   0.305 us    |        __mod_memcg_state();
 2)               |        __memcg_kmem_charge_page() {
 2)   0.212 us    |          get_mem_cgroup_from_mm();
 2)   0.705 us    |          __memcg_kmem_charge();
 2)   1.572 us    |        }
 2)   0.211 us    |        __mod_memcg_state();
 2)               |        __memcg_kmem_charge_page() {
 2)   0.208 us    |          get_mem_cgroup_from_mm();
 2)   0.685 us    |          __memcg_kmem_charge();
 2)   1.604 us    |        }
 2)   0.209 us    |        __mod_memcg_state();
 2)               |        __memcg_kmem_charge_page() {
 2)   0.202 us    |          get_mem_cgroup_from_mm();
 2)   0.697 us    |          __memcg_kmem_charge();
 2)   1.540 us    |        }
 2)   0.209 us    |        __mod_memcg_state();
 2)               |        arch_dup_task_struct() {
 2)   0.850 us    |          fpu__copy();
 2)   2.422 us    |        }
 2)               |        get_random_u64() {
 2)   0.455 us    |          _warn_unseeded_randomness();
 2)   0.423 us    |          _raw_spin_lock_irqsave();
 2)   0.355 us    |          _raw_spin_unlock_irqrestore();
 2)   2.720 us    |        }
 2)               |        account_kernel_stack() {
 2)   0.305 us    |          mod_zone_page_state();
 2)   0.335 us    |          mod_zone_page_state();
 2)   0.339 us    |          mod_zone_page_state();
 2)   0.313 us    |          mod_zone_page_state();
 2)   3.063 us    |        }
 2) + 47.225 us   |      }
 2)               |      kmem_cache_alloc_trace() {
 2)               |        _cond_resched() {
 2)   0.371 us    |          rcu_all_qs();
 2)   1.067 us    |        }
 2)   0.315 us    |        should_failslab();
 2)   0.397 us    |        memcg_kmem_put_cache();
 2)   3.587 us    |      }
 2)               |      copy_creds() {
 2)               |        prepare_creds() {
 2)   1.578 us    |          kmem_cache_alloc();
 2)   2.045 us    |          security_prepare_creds();
 2)   5.501 us    |        }
 2)   0.309 us    |        key_put();
 2)   7.808 us    |      }
 2)               |      __delayacct_tsk_init() {
 2)               |        kmem_cache_alloc() {
 2)   0.475 us    |          _cond_resched();
 2)   0.373 us    |          should_failslab();
 2)   0.635 us    |          memcg_kmem_get_cache();
 2)   0.457 us    |          memcg_kmem_put_cache();
 2)   4.112 us    |        }
 2)   5.111 us    |      }
 2)   0.381 us    |      acct_clear_integrals();
 2)   0.347 us    |      cgroup_fork();
 2)               |      sched_fork() {
 2)               |        __sched_fork() {
 2)   0.968 us    |          init_dl_task_timer();
 2)   0.676 us    |          init_dl_inactive_task_timer();
 2)   0.365 us    |          __dl_clear_params();
 2)   0.752 us    |          init_numa_balancing();
 2)   5.170 us    |        }
 2)   0.423 us    |        init_entity_runnable_average();
 2)   0.680 us    |        _raw_spin_lock_irqsave();
 2)   0.303 us    |        set_task_rq_fair();
 2)               |        task_fork_fair() {
 2)   0.329 us    |          _raw_spin_lock();
 2)   0.389 us    |          update_rq_clock();
 2)   1.335 us    |          update_curr();
 2)   1.008 us    |          place_entity();
 2)   4.816 us    |        }
 2)   0.398 us    |        _raw_spin_unlock_irqrestore();
 2) + 14.666 us   |      }
 2)   0.387 us    |      __mutex_init();
 2)   0.439 us    |      audit_alloc();
 2)               |      security_task_alloc() {
 2)               |        lsm_task_alloc() {
 2)   1.238 us    |          __kmalloc();
 2)   1.969 us    |        }
 2)   0.609 us    |        apparmor_task_alloc();
 2)   3.859 us    |      }
 2)   0.359 us    |      copy_semundo();
 2)               |      dup_fd() {
 2)               |        kmem_cache_alloc() {
 2)   0.409 us    |          _cond_resched();
 2)   0.325 us    |          should_failslab();
 2)   0.565 us    |          memcg_kmem_get_cache();
 2)   0.365 us    |          memcg_kmem_put_cache();
 2)   3.638 us    |        }
 2)   0.341 us    |        __init_waitqueue_head();
 2)   0.338 us    |        _raw_spin_lock();
 2)               |        alloc_fdtable() {
 2)   1.594 us    |          kmem_cache_alloc_trace();
 2)   2.429 us    |          kvmalloc_node();
 2)   1.770 us    |          kvmalloc_node();
 2)   7.284 us    |        }
 2)   0.387 us    |        _raw_spin_lock();
 2)   0.607 us    |        copy_fd_bitmaps();
 2) + 18.769 us   |      }
 2)               |      copy_fs_struct() {
 2)               |        kmem_cache_alloc() {
 2)   0.457 us    |          _cond_resched();
 2)   0.349 us    |          should_failslab();
 2)   0.632 us    |          memcg_kmem_get_cache();
 2)   0.401 us    |          memcg_kmem_put_cache();
 2)   3.823 us    |        }
 2)   0.393 us    |        _raw_spin_lock();
 2)               |        path_get() {
 2)   0.795 us    |          mntget();
 2)   2.012 us    |        }
 2)               |        path_get() {
 2)   0.379 us    |          mntget();
 2)   1.223 us    |        }
 2)   9.759 us    |      }
 2)               |      kmem_cache_alloc() {
 2)               |        _cond_resched() {
 2)   0.685 us    |          rcu_all_qs();
 2)   1.347 us    |        }
 2)   0.384 us    |        should_failslab();
 2)   0.608 us    |        memcg_kmem_get_cache();
 2)   0.429 us    |        memcg_kmem_put_cache();
 2)   4.853 us    |      }
 2)   0.401 us    |      _raw_spin_lock_irq();
 2)               |      kmem_cache_alloc() {
 2)               |        _cond_resched() {
 2)   0.379 us    |          rcu_all_qs();
 2)   1.082 us    |        }
 2)   0.271 us    |        should_failslab();
 2)   0.507 us    |        memcg_kmem_get_cache();
 2)   0.381 us    |        memcg_kmem_put_cache();
 2)   4.657 us    |      }
 2)   0.351 us    |      __init_waitqueue_head();
 2)               |      hrtimer_init() {
 2)   0.395 us    |        __hrtimer_init();
 2)   1.107 us    |      }
 2)   0.369 us    |      _raw_spin_lock();
 2)   0.405 us    |      posix_cputimers_group_init();
 2)   0.419 us    |      tty_audit_fork();
 2)               |      sched_autogroup_fork() {
 2)               |        __lock_task_sighand() {
 2)   0.357 us    |          _raw_spin_lock_irqsave();
 2)   1.044 us    |        }
 2)   0.357 us    |        _raw_spin_unlock_irqrestore();
 2)   2.722 us    |      }
 2)   0.371 us    |      __mutex_init();
 2)   0.369 us    |      __mutex_init();
 2)               |      dup_mm() {
 2)               |        kmem_cache_alloc() {
 2)   0.465 us    |          _cond_resched();
 2)   0.350 us    |          should_failslab();
 2)   0.578 us    |          memcg_kmem_get_cache();
 2)   0.355 us    |          memcg_kmem_put_cache();
 2)   3.857 us    |        }
 2)               |        mm_init() {
 2)   0.346 us    |          __init_rwsem();
 2)   7.583 us    |          pgd_alloc();
 2)   0.493 us    |          __mutex_init();
 2)   0.234 us    |          __init_rwsem();
 2) + 10.496 us   |        }
 2)               |        dup_mmap() {
 2)   0.547 us    |          uprobe_start_dup_mmap();
 2)   0.355 us    |          down_write_killable();
 2)   0.237 us    |          uprobe_dup_mmap();
 2)   0.339 us    |          down_write();
 2)   0.323 us    |          get_mm_exe_file();
 2)   1.979 us    |          vm_area_dup();
 2)   0.239 us    |          vma_dup_policy();
 2)   0.289 us    |          dup_userfaultfd();
 2)   0.349 us    |          anon_vma_fork();
 2)   0.357 us    |          vma_do_get_file();
 2)   0.731 us    |          down_write();
 2)   0.580 us    |          vma_interval_tree_insert_after();
 2)   0.346 us    |          up_write();
 2)   0.480 us    |          __vma_link_rb();
 2)   0.511 us    |          copy_page_range();
 2)   1.830 us    |          vm_area_dup();
 2)   0.383 us    |          vma_dup_policy();
 2)   0.559 us    |          dup_userfaultfd();
 2)   0.345 us    |          anon_vma_fork();
 2)   0.351 us    |          vma_do_get_file();
 2)   0.637 us    |          down_write();
 2)   0.349 us    |          vma_interval_tree_insert_after();
 2)   0.299 us    |          up_write();
 2)   0.645 us    |          __vma_link_rb();
 2)   0.371 us    |          copy_page_range();
 2)   1.680 us    |          vm_area_dup();
 2)   0.361 us    |          vma_dup_policy();
 2)   0.343 us    |          dup_userfaultfd();
 2)   0.359 us    |          anon_vma_fork();
 2)   0.368 us    |          vma_do_get_file();
 2)   0.578 us    |          down_write();
 2)   0.424 us    |          vma_interval_tree_insert_after();
 2)   0.263 us    |          up_write();
 2)   0.761 us    |          __vma_link_rb();
 2)   0.375 us    |          copy_page_range();
 2)   1.355 us    |          security_vm_enough_memory_mm();
 2)   1.546 us    |          vm_area_dup();
 2)   0.272 us    |          vma_dup_policy();
 2)   0.279 us    |          dup_userfaultfd();
 2)   5.060 us    |          anon_vma_fork();
 2)   0.280 us    |          vma_do_get_file();
 2)   0.559 us    |          down_write();
 2)   0.375 us    |          vma_interval_tree_insert_after();
 2)   0.308 us    |          up_write();
 2)   0.441 us    |          __vma_link_rb();
 2) + 14.293 us   |          copy_page_range();
 2)   0.666 us    |          security_vm_enough_memory_mm();
 2)   1.617 us    |          vm_area_dup();
 2)   0.293 us    |          vma_dup_policy();
 2)   0.333 us    |          dup_userfaultfd();
 2)   3.265 us    |          anon_vma_fork();
 2)   0.257 us    |          vma_do_get_file();
 2)   0.481 us    |          down_write();
 2)   0.315 us    |          vma_interval_tree_insert_after();
 2)   0.373 us    |          up_write();
 2)   0.610 us    |          __vma_link_rb();
 2)   3.462 us    |          copy_page_range();
 2)   0.672 us    |          security_vm_enough_memory_mm();
 2)   2.193 us    |          vm_area_dup();
 2)   0.346 us    |          vma_dup_policy();
 2)   0.381 us    |          dup_userfaultfd();
 2)   4.025 us    |          anon_vma_fork();
 2)   0.361 us    |          __vma_link_rb();
 2)   2.885 us    |          copy_page_range();
 2)   0.619 us    |          security_vm_enough_memory_mm();
 2)   1.281 us    |          vm_area_dup();
 2)   0.274 us    |          vma_dup_policy();
 2)   0.273 us    |          dup_userfaultfd();
 2)   3.258 us    |          anon_vma_fork();
 2)   0.677 us    |          __vma_link_rb();
 2) ! 100.194 us  |          copy_page_range();
 2)   1.953 us    |          vm_area_dup();
 2)   0.304 us    |          vma_dup_policy();
 2)   0.363 us    |          dup_userfaultfd();
 2)   0.457 us    |          anon_vma_fork();
 2)   0.363 us    |          vma_do_get_file();
 2)   0.678 us    |          down_write();
 2)   0.353 us    |          vma_interval_tree_insert_after();
 2)   0.345 us    |          up_write();
 2)   0.815 us    |          __vma_link_rb();
 2)   0.383 us    |          copy_page_range();
 2)   1.418 us    |          vm_area_dup();
 2)   0.409 us    |          vma_dup_policy();
 2)   0.359 us    |          dup_userfaultfd();
 2)   0.339 us    |          anon_vma_fork();
 2)   0.362 us    |          vma_do_get_file();
 2)   0.565 us    |          down_write();
 2)   0.363 us    |          vma_interval_tree_insert_after();
 2)   0.341 us    |          up_write();
 2)   0.537 us    |          __vma_link_rb();
 2)   0.445 us    |          copy_page_range();
 2)   1.375 us    |          vm_area_dup();
 2)   0.355 us    |          vma_dup_policy();
 2)   0.368 us    |          dup_userfaultfd();
 2)   0.323 us    |          anon_vma_fork();
 2)   0.298 us    |          vma_do_get_file();
 2)   0.561 us    |          down_write();
 2)   0.375 us    |          vma_interval_tree_insert_after();
 2)   0.300 us    |          up_write();
 2)   0.317 us    |          __vma_link_rb();
 2)   0.345 us    |          copy_page_range();
 2)   0.880 us    |          security_vm_enough_memory_mm();
 2)   1.674 us    |          vm_area_dup();
 2)   0.297 us    |          vma_dup_policy();
 2)   0.345 us    |          dup_userfaultfd();
 2)   4.306 us    |          anon_vma_fork();
 2)   0.303 us    |          vma_do_get_file();
 2)   0.513 us    |          down_write();
 2)   0.291 us    |          vma_interval_tree_insert_after();
 2)   0.275 us    |          up_write();
 2)   0.443 us    |          __vma_link_rb();
 2) + 13.459 us   |          copy_page_range();
 2)   0.750 us    |          security_vm_enough_memory_mm();
 2)   1.241 us    |          vm_area_dup();
 2)   0.391 us    |          vma_dup_policy();
 2)   0.319 us    |          dup_userfaultfd();
 2)   3.446 us    |          anon_vma_fork();
 2)   0.306 us    |          vma_do_get_file();
 2)   0.609 us    |          down_write();
 2)   0.379 us    |          vma_interval_tree_insert_after();
 2)   0.337 us    |          up_write();
 2)   0.578 us    |          __vma_link_rb();
 2)   1.926 us    |          copy_page_range();
 2)   0.742 us    |          security_vm_enough_memory_mm();
 2)   1.637 us    |          vm_area_dup();
 2)   0.355 us    |          vma_dup_policy();
 2)   0.337 us    |          dup_userfaultfd();
 2)   0.354 us    |          anon_vma_fork();
 2)   0.570 us    |          __vma_link_rb();
 2)   0.335 us    |          copy_page_range();
 2)   2.260 us    |          vm_area_dup();
 2)   0.345 us    |          vma_dup_policy();
 2)   0.367 us    |          dup_userfaultfd();
 2)   0.342 us    |          anon_vma_fork();
 2)   0.381 us    |          vma_do_get_file();
 2)   0.625 us    |          down_write();
 2)   0.417 us    |          vma_interval_tree_insert_after();
 2)   0.354 us    |          up_write();
 2)   0.505 us    |          __vma_link_rb();
 2)   0.377 us    |          copy_page_range();
 2)   0.730 us    |          security_vm_enough_memory_mm();
 2)   1.373 us    |          vm_area_dup();
 2)   0.344 us    |          vma_dup_policy();
 2)   0.363 us    |          dup_userfaultfd();
 2)   4.075 us    |          anon_vma_fork();
 2)   0.471 us    |          __vma_link_rb();
 2)   6.130 us    |          copy_page_range();
 2)   1.875 us    |          vm_area_dup();
 2)   0.331 us    |          vma_dup_policy();
 2)   0.359 us    |          dup_userfaultfd();
 2)   0.324 us    |          anon_vma_fork();
 2)   0.359 us    |          vma_do_get_file();
 2)   0.734 us    |          down_write();
 2)   1.128 us    |          vma_interval_tree_insert_after();
 2)   0.361 us    |          up_write();
 2)   0.559 us    |          __vma_link_rb();
 2)   0.350 us    |          copy_page_range();
 2)   1.407 us    |          vm_area_dup();
 2)   0.359 us    |          vma_dup_policy();
 2)   0.347 us    |          dup_userfaultfd();
 2)   0.407 us    |          anon_vma_fork();
 2)   0.359 us    |          vma_do_get_file();
 2)   0.584 us    |          down_write();
 2)   0.782 us    |          vma_interval_tree_insert_after();
 2)   0.347 us    |          up_write();
 2)   0.820 us    |          __vma_link_rb();
 2)   0.341 us    |          copy_page_range();
 2)   1.211 us    |          vm_area_dup();
 2)   0.334 us    |          vma_dup_policy();
 2)   0.323 us    |          dup_userfaultfd();
 2)   0.300 us    |          anon_vma_fork();
 2)   0.297 us    |          vma_do_get_file();
 2)   0.449 us    |          down_write();
 2)   0.756 us    |          vma_interval_tree_insert_after();
 2)   0.307 us    |          up_write();
 2)   0.670 us    |          __vma_link_rb();
 2)   0.347 us    |          copy_page_range();
 2)   1.114 us    |          vm_area_dup();
 2)   0.291 us    |          vma_dup_policy();
 2)   0.325 us    |          dup_userfaultfd();
 2)   0.307 us    |          anon_vma_fork();
 2)   0.326 us    |          vma_do_get_file();
 2)   0.527 us    |          down_write();
 2)   0.367 us    |          vma_interval_tree_insert_after();
 2)   0.340 us    |          up_write();
 2)   0.459 us    |          __vma_link_rb();
 2)   0.339 us    |          copy_page_range();
 2)   0.808 us    |          security_vm_enough_memory_mm();
 2)   1.199 us    |          vm_area_dup();
 2)   0.308 us    |          vma_dup_policy();
 2)   0.327 us    |          dup_userfaultfd();
 2)   4.306 us    |          anon_vma_fork();
 2)   0.365 us    |          vma_do_get_file();
 2)   0.568 us    |          down_write();
 2)   0.475 us    |          vma_interval_tree_insert_after();
 2)   0.391 us    |          up_write();
 2)   0.573 us    |          __vma_link_rb();
 2)   6.521 us    |          copy_page_range();
 2)   0.766 us    |          security_vm_enough_memory_mm();
 2)   1.319 us    |          vm_area_dup();
 2)   0.359 us    |          vma_dup_policy();
 2)   0.307 us    |          dup_userfaultfd();
 2)   4.019 us    |          anon_vma_fork();
 2)   0.379 us    |          vma_do_get_file();
 2)   0.525 us    |          down_write();
 2)   1.080 us    |          vma_interval_tree_insert_after();
 2)   0.274 us    |          up_write();
 2)   0.517 us    |          __vma_link_rb();
 2)   1.964 us    |          copy_page_range();
 2)   0.681 us    |          security_vm_enough_memory_mm();
 2)   1.129 us    |          vm_area_dup();
 2)   0.391 us    |          vma_dup_policy();
 2)   0.283 us    |          dup_userfaultfd();
 2)   3.847 us    |          anon_vma_fork();
 2)   0.441 us    |          __vma_link_rb();
 2)   2.770 us    |          copy_page_range();
 2)   1.345 us    |          vm_area_dup();
 2)   0.373 us    |          vma_dup_policy();
 2)   0.645 us    |          dup_userfaultfd();
 2)   0.361 us    |          anon_vma_fork();
 2)   0.351 us    |          vma_do_get_file();
 2)   0.643 us    |          down_write();
 2)   0.433 us    |          vma_interval_tree_insert_after();
 2)   0.349 us    |          up_write();
 2)   0.516 us    |          __vma_link_rb();
 2)   0.647 us    |          copy_page_range();
 2)   1.110 us    |          vm_area_dup();
 2)   0.397 us    |          vma_dup_policy();
 2)   0.660 us    |          dup_userfaultfd();
 2)   0.317 us    |          anon_vma_fork();
 2)   0.318 us    |          vma_do_get_file();
 2)   0.477 us    |          down_write();
 2)   1.093 us    |          vma_interval_tree_insert_after();
 2)   0.335 us    |          up_write();
 2)   0.497 us    |          __vma_link_rb();
 2)   0.389 us    |          copy_page_range();
 2)   1.181 us    |          vm_area_dup();
 2)   0.313 us    |          vma_dup_policy();
 2)   0.347 us    |          dup_userfaultfd();
 2)   0.319 us    |          anon_vma_fork();
 2)   0.329 us    |          vma_do_get_file();
 2)   0.577 us    |          down_write();
 2)   0.433 us    |          vma_interval_tree_insert_after();
 2)   0.631 us    |          up_write();
 2)   0.419 us    |          __vma_link_rb();
 2)   0.300 us    |          copy_page_range();
 2)   0.674 us    |          security_vm_enough_memory_mm();
 2)   0.971 us    |          vm_area_dup();
 2)   0.335 us    |          vma_dup_policy();
 2)   0.373 us    |          dup_userfaultfd();
 2)   4.171 us    |          anon_vma_fork();
 2)   0.353 us    |          vma_do_get_file();
 2)   0.486 us    |          down_write();
 2)   0.336 us    |          vma_interval_tree_insert_after();
 2)   0.361 us    |          up_write();
 2)   0.571 us    |          __vma_link_rb();
 2)   1.471 us    |          copy_page_range();
 2)   0.704 us    |          security_vm_enough_memory_mm();
 2)   1.091 us    |          vm_area_dup();
 2)   0.344 us    |          vma_dup_policy();
 2)   0.307 us    |          dup_userfaultfd();
 2)   3.742 us    |          anon_vma_fork();
 2)   0.347 us    |          vma_do_get_file();
 2)   0.605 us    |          down_write();
 2)   0.301 us    |          vma_interval_tree_insert_after();
 2)   0.410 us    |          up_write();
 2)   0.533 us    |          __vma_link_rb();
 2)   1.518 us    |          copy_page_range();
 2)   1.197 us    |          vm_area_dup();
 2)   0.341 us    |          vma_dup_policy();
 2)   0.327 us    |          dup_userfaultfd();
 2)   0.347 us    |          anon_vma_fork();
 2)   0.312 us    |          vma_do_get_file();
 2)   0.633 us    |          down_write();
 2)   0.457 us    |          vma_interval_tree_insert_after();
 2)   0.307 us    |          up_write();
 2)   0.453 us    |          __vma_link_rb();
 2)   0.363 us    |          copy_page_range();
 2)   1.237 us    |          vm_area_dup();
 2)   0.350 us    |          vma_dup_policy();
 2)   0.341 us    |          dup_userfaultfd();
 2)   0.304 us    |          anon_vma_fork();
 2)   0.355 us    |          vma_do_get_file();
 2)   0.555 us    |          down_write();
 2)   0.395 us    |          vma_interval_tree_insert_after();
 2)   0.327 us    |          up_write();
 2)   0.455 us    |          __vma_link_rb();
 2)   0.329 us    |          copy_page_range();
 2)   1.093 us    |          vm_area_dup();
 2)   0.377 us    |          vma_dup_policy();
 2)   0.330 us    |          dup_userfaultfd();
 2)   0.357 us    |          anon_vma_fork();
 2)   0.361 us    |          vma_do_get_file();
 2)   0.582 us    |          down_write();
 2)   0.377 us    |          vma_interval_tree_insert_after();
 2)   0.361 us    |          up_write();
 2)   0.483 us    |          __vma_link_rb();
 2)   0.373 us    |          copy_page_range();
 2)   0.740 us    |          security_vm_enough_memory_mm();
 2)   1.325 us    |          vm_area_dup();
 2)   0.369 us    |          vma_dup_policy();
 2)   0.365 us    |          dup_userfaultfd();
 2)   4.446 us    |          anon_vma_fork();
 2)   0.403 us    |          vma_do_get_file();
 2)   0.567 us    |          down_write();
 2)   0.320 us    |          vma_interval_tree_insert_after();
 2)   0.305 us    |          up_write();
 2)   0.527 us    |          __vma_link_rb();
 2)   2.514 us    |          copy_page_range();
 2)   0.723 us    |          security_vm_enough_memory_mm();
 2)   1.039 us    |          vm_area_dup();
 2)   0.341 us    |          vma_dup_policy();
 2)   0.294 us    |          dup_userfaultfd();
 2)   3.428 us    |          anon_vma_fork();
 2)   0.307 us    |          vma_do_get_file();
 2)   0.507 us    |          down_write();
 2)   0.387 us    |          vma_interval_tree_insert_after();
 2)   0.311 us    |          up_write();
 2)   0.529 us    |          __vma_link_rb();
 2)   1.361 us    |          copy_page_range();
 2)   0.684 us    |          security_vm_enough_memory_mm();
 2)   1.145 us    |          vm_area_dup();
 2)   0.335 us    |          vma_dup_policy();
 2)   0.335 us    |          dup_userfaultfd();
 2)   3.522 us    |          anon_vma_fork();
 2)   0.457 us    |          __vma_link_rb();
 2)   1.537 us    |          copy_page_range();
 2)   1.177 us    |          vm_area_dup();
 2)   0.343 us    |          vma_dup_policy();
 2)   0.341 us    |          dup_userfaultfd();
 2)   0.289 us    |          anon_vma_fork();
 2)   0.326 us    |          vma_do_get_file();
 2)   0.682 us    |          down_write();
 2)   0.335 us    |          vma_interval_tree_insert_after();
 2)   0.311 us    |          up_write();
 2)   0.729 us    |          __vma_link_rb();
 2)   0.319 us    |          copy_page_range();
 2)   1.875 us    |          vm_area_dup();
 2)   0.323 us    |          vma_dup_policy();
 2)   0.329 us    |          dup_userfaultfd();
 2)   0.323 us    |          anon_vma_fork();
 2)   0.328 us    |          vma_do_get_file();
 2)   0.629 us    |          down_write();
 2)   1.061 us    |          vma_interval_tree_insert_after();
 2)   0.272 us    |          up_write();
 2)   0.435 us    |          __vma_link_rb();
 2)   0.278 us    |          copy_page_range();
 2)   1.293 us    |          vm_area_dup();
 2)   0.409 us    |          vma_dup_policy();
 2)   0.314 us    |          dup_userfaultfd();
 2)   0.327 us    |          anon_vma_fork();
 2)   0.254 us    |          vma_do_get_file();
 2)   0.553 us    |          down_write();
 2)   0.778 us    |          vma_interval_tree_insert_after();
 2)   0.330 us    |          up_write();
 2)   0.509 us    |          __vma_link_rb();
 2)   0.581 us    |          copy_page_range();
 2)   1.227 us    |          vm_area_dup();
 2)   0.319 us    |          vma_dup_policy();
 2)   0.344 us    |          dup_userfaultfd();
 2)   0.333 us    |          anon_vma_fork();
 2)   0.344 us    |          vma_do_get_file();
 2)   0.622 us    |          down_write();
 2)   0.816 us    |          vma_interval_tree_insert_after();
 2)   0.397 us    |          up_write();
 2)   0.509 us    |          __vma_link_rb();
 2)   0.315 us    |          copy_page_range();
 2)   0.740 us    |          security_vm_enough_memory_mm();
 2)   1.085 us    |          vm_area_dup();
 2)   0.325 us    |          vma_dup_policy();
 2)   0.291 us    |          dup_userfaultfd();
 2)   4.031 us    |          anon_vma_fork();
 2)   0.321 us    |          vma_do_get_file();
 2)   0.590 us    |          down_write();
 2)   0.879 us    |          vma_interval_tree_insert_after();
 2)   0.387 us    |          up_write();
 2)   0.697 us    |          __vma_link_rb();
 2)   1.496 us    |          copy_page_range();
 2)   0.683 us    |          security_vm_enough_memory_mm();
 2)   1.167 us    |          vm_area_dup();
 2)   0.337 us    |          vma_dup_policy();
 2)   0.329 us    |          dup_userfaultfd();
 2)   3.294 us    |          anon_vma_fork();
 2)   0.338 us    |          vma_do_get_file();
 2)   0.529 us    |          down_write();
 2)   0.942 us    |          vma_interval_tree_insert_after();
 2)   0.411 us    |          up_write();
 2)   0.479 us    |          __vma_link_rb();
 2)   1.443 us    |          copy_page_range();
 2)   0.672 us    |          security_vm_enough_memory_mm();
 2)   1.141 us    |          vm_area_dup();
 2)   0.357 us    |          vma_dup_policy();
 2)   0.323 us    |          dup_userfaultfd();
 2)   3.486 us    |          anon_vma_fork();
 2)   0.653 us    |          __vma_link_rb();
 2)   1.451 us    |          copy_page_range();
 2)   0.719 us    |          security_vm_enough_memory_mm();
 2)   1.090 us    |          vm_area_dup();
 2)   0.327 us    |          vma_dup_policy();
 2)   0.325 us    |          dup_userfaultfd();
 2)   3.492 us    |          anon_vma_fork();
 2)   0.628 us    |          __vma_link_rb();
 2) + 15.929 us   |          copy_page_range();
 2)   1.175 us    |          vm_area_dup();
 2)   0.280 us    |          vma_dup_policy();
 2)   0.313 us    |          dup_userfaultfd();
 2)   0.369 us    |          anon_vma_fork();
 2)   0.678 us    |          __vma_link_rb();
 2)   1.936 us    |          copy_page_range();
 2)   1.067 us    |          vm_area_dup();
 2)   0.359 us    |          vma_dup_policy();
 2)   0.349 us    |          dup_userfaultfd();
 2)   0.360 us    |          anon_vma_fork();
 2)   0.640 us    |          __vma_link_rb();
 2)   0.423 us    |          copy_page_range();
 2)   0.916 us    |          ldt_dup_context();
 2)   0.367 us    |          up_write();
 2)   1.239 us    |          flush_tlb_mm_range();
 2)   0.389 us    |          up_write();
 2)   0.379 us    |          dup_userfaultfd_complete();
 2)   0.422 us    |          uprobe_end_dup_mmap();
 2) ! 617.943 us  |        }
 2)   0.387 us    |        try_module_get();
 2) ! 634.728 us  |      }
 2)   0.540 us    |      copy_namespaces();
 2)   0.560 us    |      copy_thread_tls();
 2)               |      alloc_pid() {
 2)               |        kmem_cache_alloc() {
 2)   0.458 us    |          _cond_resched();
 2)   0.345 us    |          should_failslab();
 2)   0.822 us    |          memcg_kmem_get_cache();
 2)   0.493 us    |          memcg_kmem_put_cache();
 2)   4.248 us    |        }
 2)   0.441 us    |        _raw_spin_lock_irq();
 2)   0.451 us    |        __init_waitqueue_head();
 2)   0.313 us    |        _raw_spin_lock_irq();
 2)   9.403 us    |      }
 2)   0.372 us    |      __mutex_init();
 2)   0.339 us    |      user_disable_single_step();
 2)               |      cgroup_can_fork() {
 2)               |        _cond_resched() {
 2)   0.360 us    |          rcu_all_qs();
 2)   1.028 us    |        }
 2)   0.436 us    |        __percpu_down_read();
 2)   0.431 us    |        _raw_spin_lock_irq();
 2)   1.076 us    |        pids_can_fork();
 2)   5.266 us    |      }
 2)   0.485 us    |      ktime_get();
 2)   0.425 us    |      ktime_get_with_offset();
 2)   0.447 us    |      _raw_write_lock_irq();
 2)   0.369 us    |      klp_copy_process();
 2)   0.423 us    |      _raw_spin_lock();
 2)   0.385 us    |      get_seccomp_filter();
 2)   0.441 us    |      attach_pid();
 2)   0.535 us    |      attach_pid();
 2)   0.318 us    |      attach_pid();
 2)   0.407 us    |      attach_pid();
 2)   0.409 us    |      proc_fork_connector();
 2)               |      cgroup_post_fork() {
 2)   0.353 us    |        _raw_spin_lock_irq();
 2)               |        css_set_move_task() {
 2)   1.072 us    |          cgroup_move_task();
 2)   2.019 us    |        }
 2)   0.464 us    |        cpuset_fork();
 2)               |        cpu_cgroup_fork() {
 2)   0.623 us    |          task_rq_lock();
 2)   0.477 us    |          update_rq_clock();
 2)   1.287 us    |          sched_change_group();
 2)   0.441 us    |          _raw_spin_unlock_irqrestore();
 2)   4.719 us    |        }
 2)   0.425 us    |        freezer_fork();
 2)               |        cgroup_css_set_put_fork() {
 2)   0.388 us    |          irq_enter_rcu();
 2)   3.129 us    |          __sysvec_apic_timer_interrupt();
 2)   0.565 us    |          irq_exit_rcu();
 2)   0.337 us    |          rcuwait_wake_up();
 2)   7.676 us    |        }
 2) + 19.058 us   |      }
 2)   0.533 us    |      uprobe_copy_process();
 2) ! 827.540 us  |    }
 2)   0.385 us    |    get_task_pid();
 2)   0.735 us    |    pid_vnr();
 2)               |    wake_up_new_task() {
 2)   0.341 us    |      _raw_spin_lock_irqsave();
 2)               |      select_task_rq_fair() {
 2)   1.600 us    |        find_idlest_group();
 2)   0.377 us    |        available_idle_cpu();
 2)   0.346 us    |        available_idle_cpu();
 2)   0.795 us    |        find_idlest_group();
 2)   5.677 us    |      }
 2)   0.371 us    |      set_task_rq_fair();
 2)               |      __task_rq_lock() {
 2)   0.359 us    |        _raw_spin_lock();
 2)   1.035 us    |      }
 2)   0.439 us    |      update_rq_clock();
 2)               |      post_init_entity_util_avg() {
 2)               |        attach_entity_cfs_rq() {
 2)   0.485 us    |          __update_load_avg_cfs_rq();
 2)   0.368 us    |          attach_entity_load_avg();
 2)   1.847 us    |          propagate_entity_cfs_rq.isra.0();
 2)   4.326 us    |        }
 2)   5.214 us    |      }
 2)               |      psi_task_change() {
 2)   0.357 us    |        psi_flags_change();
 2)               |        psi_group_change() {
 2)   0.441 us    |          record_times();
 2)   1.531 us    |        }
 2)               |        psi_group_change() {
 2)   0.375 us    |          record_times();
 2)   1.267 us    |        }
 2)               |        psi_group_change() {
 2)   0.391 us    |          record_times();
 2)   1.247 us    |        }
 2)               |        psi_group_change() {
 2)   0.359 us    |          record_times();
 2)   1.160 us    |        }
 2)               |        psi_group_change() {
 2)   0.385 us    |          record_times();
 2)   1.171 us    |        }
 2)               |        psi_group_change() {
 2)   0.335 us    |          record_times();
 2)   1.120 us    |        }
 2)               |        psi_group_change() {
 2)   0.313 us    |          record_times();
 2)   1.305 us    |        }
 2) + 12.902 us   |      }
 2)               |      enqueue_task_fair() {
 2)               |        enqueue_entity() {
 2)   0.365 us    |          update_curr();
 2)   0.370 us    |          __update_load_avg_se();
 2)   0.341 us    |          __update_load_avg_cfs_rq();
 2)   0.300 us    |          update_cfs_group();
 2)   0.405 us    |          account_entity_enqueue();
 2)   0.373 us    |          __enqueue_entity();
 2)   5.332 us    |        }
 2)               |        enqueue_entity() {
 2)   0.379 us    |          update_curr();
 2)   0.371 us    |          __update_load_avg_se();
 2)   0.632 us    |          __update_load_avg_cfs_rq();
 2)   0.549 us    |          update_cfs_group();
 2)   0.370 us    |          account_entity_enqueue();
 2)   0.421 us    |          place_entity();
 2)   0.361 us    |          __enqueue_entity();
 2)   5.940 us    |        }
 2)   0.298 us    |        hrtick_update();
 2) + 13.080 us   |      }
 2)               |      check_preempt_curr() {
 2)   0.370 us    |        resched_curr();
 2)   1.222 us    |      }
 2)   0.371 us    |      _raw_spin_unlock_irqrestore();
 2) + 46.419 us   |    }
 2)               |    put_pid() {
 2)   0.925 us    |      put_pid.part.0();
 2)   1.617 us    |    }
 2) ! 880.434 us  |  }

# sudo ./reset_ftrace
```

