#ifndef __FDQUEUE_H__
#define __FDQUEUE_H__

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define PAGESIZE 4096
#define DEFAULT_CAPACITY PAGESIZE

typedef struct fd_mmapaddr_tuple {
  int fd;
  void *mmapaddr;
} fd_mmapaddr_tuple_t;

typedef struct fd_queue {
  size_t len;
  size_t capacity;
  fd_mmapaddr_tuple_t *values;
} fd_queue_t;

size_t fdqueue_len(const fd_queue_t *fdqueue);
fd_mmapaddr_tuple_t *fdqueue_begin(const fd_queue_t *fdqueue);
fd_mmapaddr_tuple_t *fdqueue_end(const fd_queue_t *fdqueue);
fd_mmapaddr_tuple_t *fdqueue_get(const fd_queue_t *fdqueue, size_t index);

fd_queue_t *new_fdqueue();

void free_fdqueue();

void fdqueue_push(fd_queue_t *fdqueue, fd_mmapaddr_tuple_t value);

#endif