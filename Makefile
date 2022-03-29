CFLAGS = -Wall -Wextra -Ofast

main: fdqueue.o bench_vfs.o bench_mmap.o bench_vfs.h bench_mmap.h common.h
	$(CC) $(CFLAGS) -lpthread fdqueue.o bench_vfs.o bench_mmap.o main.c -o storage_bench

bench_vfs.o: fdqueue.h bench_vfs.c bench_vfs.h common.h
	$(CC) $(CFLAGS) -c bench_vfs.c -o bench_vfs.o

bench_mmap.o: fdqueue.h bench_mmap.c bench_mmap.h common.h
	$(CC) $(CFLAGS) -c bench_mmap.c -o bench_mmap.o

fdqueue.o: fdqueue.c fdqueue.h
	$(CC) $(CFLAGS) -c fdqueue.c -o fdqueue.o

clean:
	rm bench_vfs.o bench_mmap.o storage_bench; rm -rf ./files/*;