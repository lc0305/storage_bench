#include "fdqueue.h"

size_t fdqueue_len(const fd_queue_t *const fdqueue) { return fdqueue->len; }

fd_mmapaddr_tuple_t *fdqueue_begin(const fd_queue_t *const fdqueue) {
  return fdqueue->values;
}

fd_mmapaddr_tuple_t *fdqueue_end(const fd_queue_t *const fdqueue) {
  return &fdqueue->values[fdqueue_len(fdqueue)];
}

fd_mmapaddr_tuple_t *fdqueue_get(const fd_queue_t *const fdqueue,
                                 const size_t index) {
  assert(index < fdqueue_len(fdqueue));
  return &fdqueue->values[index];
}

fd_queue_t *new_fdqueue() {
  fd_queue_t *const ptr = (fd_queue_t *)malloc(sizeof(fd_queue_t));
  assert(NULL != ptr);
  ptr->len = 0;
  ptr->capacity = DEFAULT_CAPACITY / sizeof(fd_mmapaddr_tuple_t);
  ptr->values = (fd_mmapaddr_tuple_t *)malloc(DEFAULT_CAPACITY);
  assert(NULL != ptr->values);
  return ptr;
}

void fdqueue_push(fd_queue_t *const fdqueue, const fd_mmapaddr_tuple_t value) {
  if (fdqueue->len >= fdqueue->capacity) {
    fdqueue->capacity <<= 1;
    fdqueue->values = (fd_mmapaddr_tuple_t *)realloc(
        fdqueue->values, sizeof(fd_mmapaddr_tuple_t) * fdqueue->capacity);
    assert(NULL != fdqueue->values);
  }
  fdqueue->values[fdqueue->len] = value;
  fdqueue->len++;
}

void free_fdqueue(fd_queue_t *const fdqueue) {
  free(fdqueue->values);
  free(fdqueue);
}