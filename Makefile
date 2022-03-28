CFLAGS = -Wall -Wextra -Ofast -lpthread

main: bench_vfs.o bench_mmap.o bench_vfs.h bench_mmap.h common.h
	$(CC) $(CFLAGS) bench_vfs.o bench_mmap.o main.c -o storage_bench

bench_vfs.o: bench_vfs.c bench_vfs.h common.h
	$(CC) $(CFLAGS) -c bench_vfs.c -o bench_vfs.o

bench_mmap.o: bench_mmap.c bench_mmap.h common.h
	$(CC) $(CFLAGS) -c bench_mmap.c -o bench_mmap.o

clean:
	rm bench_vfs.o bench_mmap.o storage_bench; rm -rf ./files/*;