#include "bench_vfs.h"

static int write_file(const char *file_path, const uint8_t *buf,
                      const size_t buf_size, const int flags) {
  int open_flags = O_WRONLY | O_CREAT;
  if (flags & OPEN_SYNC)
    open_flags |= O_SYNC;
#ifdef __linux__
  if (flags & OPEN_DIRECT)
    open_flags |= O_DIRECT;
#endif
  const int fd = openat(AT_FDCWD, file_path, open_flags);
  if (fd < 0)
    return -1;
#ifdef __MACH__
  if ((flags & OPEN_DIRECT) && fcntl(fd, F_NOCACHE) < 0)
    return -1;
#endif
  size_t cursor = 0;
  ssize_t nbytes;
  while (0 < (nbytes = write(fd, &buf[cursor], buf_size - cursor)) &&
         (cursor += nbytes) < buf_size)
    ;
  if (nbytes < 0) {
    close(fd);
    return -1;
  }
  return fd;
}

static int close_file(const int fd, const int flags) {
  if ((flags & CLOSE_FSYNC) && fsync(fd) < 0)
    return -1;
  if (close(fd) < 0)
    return -1;
  return 0;
}

void *worker_thread_vfs(void *_ctx) {
  fd_queue_t *const fdqueue = new_fdqueue();
  const uint64_t tid = getthid();

  // spinning and waiting for all threads to start
  while (atomic_load_explicit(&is_started, memory_order_relaxed) == false)
    ;

  char file_path[256], log_buf[512];
  const int flags = bench_args.flags;
  ssize_t i;
  while ((i = atomic_fetch_add_explicit(&iteration_write, 1,
                                        memory_order_relaxed)) <
         bench_args.num_files) {
    if (i == 0)
      clock_gettime(CLOCK_MONOTONIC, &start);

    sprintf(file_path, "./files/bench-%lu", i);

    if (flags & VERBOSE) {
      write(STDOUT_FILENO, log_buf,
            sprintf(log_buf, "tid %llu is now writing %s.\n", tid, file_path));
    }

    const int fd = write_file(file_path, buf, bench_args.file_size, flags);
    if (fd < 0)
      goto err;
    fdqueue_push(fdqueue, (fd_mmapaddr_tuple_t){.fd = fd, .mmapaddr = NULL});
  }

  for (const fd_mmapaddr_tuple_t *iter = fdqueue_begin(fdqueue);
       iter != fdqueue_end(fdqueue); ++iter) {
    const int fd = iter->fd;
    if (flags & VERBOSE) {
      write(STDOUT_FILENO, log_buf,
            sprintf(log_buf, "tid %llu is now closing fd %d.\n", tid, fd));
    }
    if (close_file(fd, flags) < 0)
      goto err;
    if (atomic_fetch_add_explicit(&iteration_close, 1, memory_order_relaxed) ==
        (bench_args.num_files - 1))
      clock_gettime(CLOCK_MONOTONIC, &stop);
  }

  cpu_usage_t cpu_usage;
  if (read_cpu(tid, &cpu_usage)) {
    fprintf(stderr, "Error reading cpu usage.\n");
    goto err;
  }
  atomic_fetch_add_explicit(&total_cpu_usr, cpu_usage.usr_usec,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&total_cpu_sys, cpu_usage.sys_usec,
                            memory_order_relaxed);
  goto ret;
err:
  fprintf(stderr, "something went wrong :(\n");
  perror(NULL);
ret:
  free_fdqueue(fdqueue);
  return NULL;
}
