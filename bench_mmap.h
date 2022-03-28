#ifndef __BENCH_MMAP_H__
#define __BENCH_MMAP_H__

#include "common.h"
#include <fcntl.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>

void *worker_thread_mmap(void *ctx);

#endif