CC = gcc
CFLAGS = -O3 -march=native -fopenmp -Wall -Wextra
LDFLAGS = -lm

all: P1_pi sequencial P1_bench P1_bench_simd

P1_pi: paralelo.c
	$(CC) $(CFLAGS) paralelo.c -o P1_pi $(LDFLAGS)

sequencial: sequencial.c
	$(CC) $(CFLAGS) sequencial.c -o sequencial $(LDFLAGS)

P1_bench: paralelo.c
	$(CC) $(CFLAGS) -DBENCHMARK paralelo.c -o P1_bench $(LDFLAGS)

P1_bench_simd: paralelo.c
	$(CC) $(CFLAGS) -DBENCHMARK -DP1_SIMD paralelo.c -o P1_bench_simd $(LDFLAGS)

clean:
	rm -f P1_pi sequencial P1_bench P1_bench_simd
