CC = gcc
CFLAGS = -O3 -march=native -fopenmp -Wall -Wextra

all: P4_histogram sequencial P4_bench P4_bench_B P4_bench_atomic

P4_histogram: paralelo.c
	$(CC) $(CFLAGS) paralelo.c -o P4_histogram

sequencial: sequencial.c
	$(CC) $(CFLAGS) sequencial.c -o sequencial

P4_bench: paralelo.c
	$(CC) $(CFLAGS) -DBENCHMARK paralelo.c -o P4_bench

P4_bench_B: paralelo.c
	$(CC) $(CFLAGS) -DBENCHMARK -DP4_ABORDAGEM_B paralelo.c -o P4_bench_B

P4_bench_atomic: paralelo.c
	$(CC) $(CFLAGS) -DBENCHMARK -DP4_ATOMIC paralelo.c -o P4_bench_atomic

clean:
	rm -f P4_histogram sequencial P4_bench P4_bench_B P4_bench_atomic