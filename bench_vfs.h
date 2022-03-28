#ifndef __BENCH_VFS_H__
#define __BENCH_VFS_H__

#include "common.h"
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#define HAVE_ARCH_STRUCT_FLOCK
#ifdef __linux__
#include <linux/fcntl.h>
#endif
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>

void *worker_thread_vfs(void *ctx);

#endif