CC = gcc
CFLAGS = -O3 -march=native -fopenmp -Wall -Wextra

all: P4_histogram sequencial P4_bench

P4_histogram: paralelo.c
	$(CC) $(CFLAGS) paralelo.c -o P4_histogram

sequencial: sequencial.c
	$(CC) $(CFLAGS) sequencial.c -o sequencial

P4_bench: paralelo.c
	$(CC) $(CFLAGS) -DBENCHMARK paralelo.c -o P4_bench

clean:
	rm -f P4_histogram sequencial P4_bench
