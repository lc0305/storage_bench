#ifndef __BENCH_COMMON_H__
#define __BENCH_COMMON_H__

#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/resource.h>
#endif
#ifdef __MACH__
#include <libproc.h>
#endif

#define PAGE_SIZE (1 << 12)
#define MICROSECONDS_IN_ONE_SECOND 1000000
#define NANOSECONDS_IN_ONE_SECOND (MICROSECONDS_IN_ONE_SECOND * 1000)
#define MAX_NUM_THREADS 64
#define OPEN_DIRECT (1 << 0)
#define OPEN_SYNC (1 << 1)
#define CLOSE_FSYNC (1 << 2)
#define CLOSE_MSYNC (1 << 3)
#define WRITE_VFS (1 << 4)
#define WRITE_MMAP (1 << 5)
#define USE_URING (1 << 6)

typedef struct bench_args {
  int num_threads, num_files, flags;
  size_t file_size;
} bench_args_t;

extern bench_args_t bench_args;
extern pthread_t threads[MAX_NUM_THREADS];
extern struct timespec start, stop;
extern _Atomic size_t iteration;
extern _Atomic bool is_started;
extern _Atomic uint64_t total_cpu_usr;
extern _Atomic uint64_t total_cpu_sys;
extern uint8_t *buf;

static inline uint64_t gettid() {
#ifdef __linux__
  const pid_t tid = syscall(SYS_gettid);
#elif defined(__MACH__)
  uint64_t tid;
  pthread_threadid_np(NULL, &tid);
#endif
  return tid;
}

typedef struct cpu_usage {
  uint64_t usr, sys;
} cpu_usage_t;

#if defined(__linux__) && 0
static inline int read_cpu(const uint64_t tid, cpu_usage_t *const cpu_usage) {
  char proc_file_path_buf[256];
  snprintf(proc_file_path_buf, sizeof(proc_file_path_buf),
           "/proc/self/task/%d/stat", (int)tid);
  const int proc_fd = open(proc_file_path_buf, O_RDONLY);
  if (proc_fd < 0)
    goto err_open;
  {
    char proc_stat_buf[512];
    if (read(proc_fd, proc_stat_buf, sizeof(proc_stat_buf)) < 0)
      goto err_read;
    proc_stat_buf[511] = 0;
    uint64_t utime, stime;
    if (sscanf(proc_stat_buf,
               "%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %lu %lu",
               &utime, &stime) == 2) {
      const uint64_t clk_ticks_per_s = sysconf(_SC_CLK_TCK);
      cpu_usage->usr = utime / clk_ticks_per_s;
      cpu_usage->sys = stime / clk_ticks_per_s;
      close(proc_fd);
      return 0;
    }
  }
err_read:
  close(proc_fd);
err_open:
  return -1;
}
#endif

#ifdef __linux__
static inline int read_cpu(const uint64_t _tid, cpu_usage_t *const cpu_usage) {
  struct rusage usage;
  if (getrusage(RUSAGE_THREAD, &usage) < 0)
    return -1;
  const timeval utime = usage.ru_utime, stime = usage.ru_stime;
  assert((ULONG_MAX / MICROSECONDS_IN_ONE_SECOND) > (uint64_t)utime.tv_sec);
  assert((ULONG_MAX / MICROSECONDS_IN_ONE_SECOND) > (uint64_t)stime.tv_sec);
  cpu_usage->usr =
      (uint64_t)utime.tv_sec * MICROSECONDS_IN_ONE_SECOND + utime.tv_usec;
  cpu_usage->sys =
      (uint64_t)stime.tv_sec * MICROSECONDS_IN_ONE_SECOND + stime.tv_usec;
  return 0;
}
#endif

#ifdef __MACH__
static inline int read_cpu(const uint64_t tid, cpu_usage_t *const cpu_usage) {
  struct proc_threadinfo pth;
  if (PROC_PIDTHREADINFO_SIZE == proc_pidinfo(getpid(), PROC_PIDTHREADID64INFO,
                                              tid, &pth,
                                              PROC_PIDTHREADINFO_SIZE)) {
    cpu_usage->usr = pth.pth_user_time / 1000;
    cpu_usage->sys = pth.pth_system_time / 1000;
    return 0;
  }
  return -1;
}
#endif

static inline struct timespec delta(struct timespec start,
                                    struct timespec end) {
  struct timespec temp;
  if ((end.tv_nsec - start.tv_nsec) < 0) {
    temp.tv_sec = end.tv_sec - start.tv_sec - 1;
    temp.tv_nsec = NANOSECONDS_IN_ONE_SECOND + end.tv_nsec - start.tv_nsec;
  } else {
    temp.tv_sec = end.tv_sec - start.tv_sec;
    temp.tv_nsec = end.tv_nsec - start.tv_nsec;
  }
  return temp;
}

static inline long timespec_to_total_nanoseconds(struct timespec timespec) {
  assert((LONG_MAX / NANOSECONDS_IN_ONE_SECOND) > timespec.tv_sec);
  return timespec.tv_sec * NANOSECONDS_IN_ONE_SECOND + timespec.tv_nsec;
}

static inline void print_throughput(const bench_args_t *const bench_args,
                                    struct timespec start,
                                    struct timespec end) {
  const double delta_in_ns =
      (double)timespec_to_total_nanoseconds(delta(start, end));

  const double duration = delta_in_ns / 10e6;
  const double throughput =
      ((double)(bench_args->file_size * bench_args->num_files)) / delta_in_ns;

  printf("duration: %.3f ms\n", duration);
  printf("throughput: %.3f GB/s\n", throughput);
}

#endif
