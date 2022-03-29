#include "bench_mmap.h"
#include "bench_vfs.h"
#include "common.h"
#include <getopt.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static const char *const shortopts = "Df:n:FhmMSt:uv";
static const struct option long_options[] = {
    {"direct", no_argument, NULL, 'D'},
    {"filesize", required_argument, NULL, 'f'},
    {"filesnum", required_argument, NULL, 'n'},
    {"fsync", no_argument, NULL, 'F'},
    {"help", no_argument, NULL, 'h'},
    {"mmap", no_argument, NULL, 'm'},
    {"msync", no_argument, NULL, 'M'},
    {"sync", no_argument, NULL, 'S'},
    {"threads", required_argument, NULL, 't'},
    {"uring", no_argument, NULL, 'u'},
    {"vfs", no_argument, NULL, 'v'},
    {NULL, 0, NULL, 0},
};
static const char *const usage =
    "Usage: %s [--direct (-D)] [--filesize (-f) <log2 of filesize in bytes>] "
    "[--filesnum (-n) <number of files>] [--fsync (-F)] [--help (-h)] [--mmap "
    "(-m)] [--msync (-M)] [--sync (-S)] [--threads (-t) <number threads>] "
    "[--uring "
    "(-u)] [--vfs (-v)]\n";

static inline int read_bench_args(const int argc, char *const *argv,
                                  bench_args_t *const bench_args) {
  int option_index = 0, opt;
  while ((opt = getopt_long(argc, argv, shortopts, long_options,
                            &option_index)) != -1) {
    switch (opt) {
    case 'D':
      bench_args->flags |= OPEN_DIRECT;
      break;
    case 'f':
      bench_args->file_size = (1 << atoi(optarg));
      break;
    case 'n':
      bench_args->num_files = atoi(optarg);
      break;
    case 'F':
      bench_args->flags |= CLOSE_FSYNC;
      break;
    case 'h':
      printf(usage, argv[0]);
      exit(EXIT_SUCCESS);
      break;
    case 'm':
      bench_args->flags &= ~(WRITE_VFS | OPEN_DIRECT);
      bench_args->flags |= WRITE_MMAP;
      break;
    case 'M':
      bench_args->flags |= CLOSE_MSYNC;
      break;
    case 'S':
      bench_args->flags |= OPEN_SYNC;
      break;
    case 't': {
      int num_threads = atoi(optarg);
      if (num_threads > MAX_NUM_THREADS)
        num_threads = MAX_NUM_THREADS;
      bench_args->num_threads = num_threads;
      break;
    }
    case 'u':
      bench_args->flags &= ~(WRITE_MMAP | CLOSE_MSYNC);
      bench_args->flags |= USE_URING;
      break;
    case 'v':
      bench_args->flags &= ~(WRITE_MMAP | CLOSE_MSYNC);
      bench_args->flags |= WRITE_VFS;
      break;
    default:
      fprintf(stderr, usage, argv[0]);
      exit(EXIT_FAILURE);
    }
  }
  return 0;
}

static inline void print_config(const bench_args_t *const bench_args) {
  printf("Writing %d %lu byte files using %d threads.\n", bench_args->num_files,
         bench_args->file_size, bench_args->num_threads);
  if (bench_args->flags & USE_URING)
    printf("Uring enabled (only supported on new Linux Kernel builds).\n");
  if (bench_args->flags & WRITE_VFS)
    printf("Writing using the VFS.\n");
  if (bench_args->flags & WRITE_MMAP)
    printf("Writing directly to pagecache using mmap.\n");
  if (bench_args->flags & OPEN_DIRECT)
    printf("Direct I/O enabled.\n");
  if (bench_args->flags & OPEN_SYNC)
    printf("Sync I/O enabled.\n");
  if (bench_args->flags & CLOSE_FSYNC)
    printf("FSync before close enabled.\n");
  if (bench_args->flags & CLOSE_MSYNC)
    printf("MSync before unmap enabled.\n");
  fflush(stdout);
}

bench_args_t bench_args = (bench_args_t){
    .num_threads = 1,
    .file_size = (1ul << 26),
    .num_files = 30,
    .flags = WRITE_VFS,
};
pthread_t threads[MAX_NUM_THREADS];
struct timespec start = {0}, stop = {0};
_Atomic size_t iteration = 0;
_Atomic bool is_started = false;
_Atomic uint64_t total_cpu_usr = 0;
_Atomic uint64_t total_cpu_sys = 0;
uint8_t *buf = NULL;

/**
 * code is leaking file descriptos left and right
 **/
static inline int run_bench(void *(*worker_thread)(void *)) {
  srand(time(NULL));

  mkdirat(AT_FDCWD, "./files", S_IRWXU | S_IRWXG | S_IROTH);

  buf = (uint8_t *)mmap(NULL, bench_args.file_size, PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  if (buf == MAP_FAILED)
    goto err;

  // trigger page fault
  for (size_t i = 0; i < bench_args.file_size; i += PAGE_SIZE) {
    buf[i] = ((uint8_t)rand());
  }

  for (int i = 1; i < bench_args.num_threads; ++i) {
    pthread_t tid;
    if (pthread_create(&tid, NULL, worker_thread, NULL) != 0)
      goto err;
    threads[i] = tid;
  }

  atomic_store_explicit(&is_started, true, memory_order_relaxed);
  worker_thread(NULL);

  for (int i = 1; i < bench_args.num_threads; ++i) {
    if (pthread_join(threads[i], NULL) != 0)
      goto err;
  }

  char file_path[256];
  for (size_t i = 0; i < bench_args.num_files; ++i) {
    sprintf(file_path, "./files/bench-%lu", i);
    if (unlink(file_path) < 0)
      goto err;
  }

  munmap(buf, bench_args.file_size);

  print_throughput(&bench_args, start, stop);
  printf("total cpu usage (in microseconds): %lu (usr: %lu, sys: %lu)\n",
         total_cpu_usr + total_cpu_sys, total_cpu_usr, total_cpu_sys);

  return 0;

err:
  fprintf(stderr, "something went wrong :(\n");
  perror(NULL);
  return 1;
}

int main(const int argc, char *const *const argv) {
  if (read_bench_args(argc, argv, &bench_args))
    return 1;
  print_config(&bench_args);
  void *(*worker_thread)(void *) = worker_thread_vfs;
  if (bench_args.flags & WRITE_MMAP)
    worker_thread = worker_thread_mmap;
  return run_bench(worker_thread);
}