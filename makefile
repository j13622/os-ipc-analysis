CC = gcc

all: gen-mat IPC-pipe IPC-shmem verify

gen-mat: gen-mat.c
	$(CC) -o gen-mat gen-mat.c

IPC-pipe: IPC-pipe.c
	$(CC) -o IPC-pipe IPC-pipe.c

IPC-shmem: IPC-shmem.c
	$(CC) -o IPC-shmem IPC-shmem.c

verify: verify.c
	$(CC) -o verify verify.c -lm

clean:
	rm -f IPC-pipe IPC-shmem verify gen-mat