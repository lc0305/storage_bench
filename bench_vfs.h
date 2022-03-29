#ifndef __BENCH_VFS_H__
#define __BENCH_VFS_H__

#include "common.h"
#define HAVE_ARCH_STRUCT_FLOCK
#ifdef __linux__
#include <linux/fcntl.h>
#else
#include <fcntl.h>
#endif
#include "fdqueue.h"
#include <inttypes.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

void *worker_thread_vfs(void *ctx);

#endif