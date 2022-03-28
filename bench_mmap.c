#include "bench_mmap.h"

typedef struct {
  int fd;
  void *mmapaddr;
} fd_mmapaddr_tuple_t;

static int write_file(const char *file_path, const uint8_t *buf,
                      const size_t buf_size,
                      fd_mmapaddr_tuple_t *fd_mmapaddr_tuple) {
  const int fd = openat(AT_FDCWD, file_path, O_CREAT | O_RDWR | O_TRUNC, 0666);
  if (fd < 0)
    return -1;
#ifdef __linux__
  if (posix_fallocate(fd, 0, buf_size) < 0)
    return -1;
#endif
#if __MACH__
  // darwin (MACH) does not implement posix_fallocate
  // this is an equivalent using fcntl
  fstore_t store = {F_ALLOCATECONTIG, F_PEOFPOSMODE, 0, buf_size};
  if (fcntl(fd, F_PREALLOCATE, &store) < 0) {
    // OK, perhaps we are too fragmented, allocate non-continuous
    store.fst_flags = F_ALLOCATEALL;
    if (fcntl(fd, F_PREALLOCATE, &store) < 0)
      return -1;
  }
  ftruncate(fd, buf_size);
#endif
  uint8_t *const file_map = (uint8_t *const)mmap(
      NULL, buf_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (file_map == MAP_FAILED)
    return -1;
  memcpy(file_map, buf, buf_size);
  fd_mmapaddr_tuple->fd = fd;
  fd_mmapaddr_tuple->mmapaddr = file_map;
  return 0;
}

static int close_file(const int fd, uint8_t *file_map, const size_t file_size,
                      const int flags) {
  if ((flags & CLOSE_MSYNC) && msync(file_map, file_size, MS_SYNC) < 0)
    return -1;
  if ((flags & CLOSE_FSYNC) && fsync(fd) < 0)
    return -1;
  if (munmap(file_map, file_size) < 0)
    return -1;
  if (close(fd) < 0)
    return -1;
  return 0;
}

void *worker_thread_mmap(void *_ctx) {
  const uint64_t tid = gettid();

  char file_path[256];
  fd_mmapaddr_tuple_t fd_mmapaddr_tuple;
  size_t i;
  // spinning and waiting for all threads to start
  while (atomic_load_explicit(&is_started, memory_order_relaxed) == false)
    ;
  while ((i = atomic_fetch_add_explicit(&iteration, 1, memory_order_relaxed)) <
         bench_args.num_files) {
    if (i == 0)
      clock_gettime(CLOCK_MONOTONIC, &start);

    sprintf(file_path, "./files/bench-%lu", i);

    printf("tid %d is now writing %s.\n", tid, file_path);
    fflush(stdout);

    const int ret =
        write_file(file_path, buf, bench_args.file_size, &fd_mmapaddr_tuple);
    if (ret < 0)
      goto err;
    const int flags = bench_args.flags;
    if (close_file(fd_mmapaddr_tuple.fd, fd_mmapaddr_tuple.mmapaddr,
                   bench_args.file_size, flags) < 0)
      goto err;
    if (i == (bench_args.num_files - 1))
      clock_gettime(CLOCK_MONOTONIC, &stop);
  }
  cpu_usage_t cpu_usage;
  if (read_cpu(tid, &cpu_usage)) {
    fprintf(stderr, "Error reading cpu usage.\n");
    goto err;
  }
  atomic_fetch_add_explicit(&total_cpu_usr, cpu_usage.usr,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&total_cpu_sys, cpu_usage.sys,
                            memory_order_relaxed);
  goto ret;
err:
  fprintf(stderr, "something went wrong :(\n");
  perror(NULL);
ret:
  return NULL;
}
