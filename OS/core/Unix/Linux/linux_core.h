#ifndef OS_LINUX_CORE_H
#define OS_LINUX_CORE_H

#include <sys/sysinfo.h>
#include <sys/sendfile.h>

#include <linux/sched.h>

#include <arpa/inet.h>

#if OS_SOUND
#  include <pulse/pulseaudio.h>
#endif

// Missing in glibc
struct lnx_sched_attr {
  u32 size;              /* Size of this structure */
  u32 sched_policy;      /* Policy (SCHED_*) */
  u64 sched_flags;       /* Flags */
  i32 sched_nice;        /* Nice value (SCHED_OTHER, SCHED_BATCH) */
  u32 sched_priority;    /* Static priority (SCHED_FIFO, SCHED_RR) */
  /* For SCHED_DEADLINE */
  u64 sched_runtime;
  u64 sched_deadline;
  u64 sched_period;
  /* Utilization hints */
  u32 sched_util_min;
  u32 sched_util_max;
};

fn void lnx_parseMeminfo(void);

fn i32 lnx_sched_setattr(u32 policy, u64 runtime_ns, u64 deadline_ns, u64 period_ns);
fn void lnx_sched_set_deadline(u64 runtime_ns, u64 deadline_ns, u64 period_ns,
                               Func_Signal *deadline_miss_handler);
fn void lnx_sched_yield(void);

#endif
