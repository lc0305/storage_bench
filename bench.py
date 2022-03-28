#!/bin/python3

import time
import os

def init_buf(buf_size):
    return b'0' * buf_size


def write_file(file_path, buf):
    f = open(file_path, "wb")
    f.write(buf)
    return f


def close_file(f, do_fsync = True):
    f.flush()
    if do_fsync:
        os.fsync(f)
    f.close()


def main():
    ratio = 0
    buf = init_buf(1 << (30 - ratio))
    iterations = 30 << ratio
    files = []

    start = time.time_ns()

    for i in range(iterations):
        file_path = f'./files/bench-{i}'
        file = write_file(file_path, buf)
        files.append(file_path)
        close_file(file, False)

    stop = time.time_ns()

    # unlink written files
    for file_path in files:
        os.remove(file_path)

    delta_in_ns = stop - start
    print(f'duration: {round(delta_in_ns / 10e6, 3)} ms')
    print(f'throughput: {round(len(buf) * iterations / delta_in_ns, 3)} GB/s')


if __name__ == '__main__':
    main()
