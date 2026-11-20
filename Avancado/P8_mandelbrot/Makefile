CC = gcc
CFLAGS = -O3 -march=native -fopenmp -Wall -Wextra

all: P8_mandelbrot sequencial P8_bench

P8_mandelbrot: paralelo.c
	$(CC) $(CFLAGS) paralelo.c -o P8_mandelbrot

sequencial: sequencial.c
	$(CC) $(CFLAGS) sequencial.c -o sequencial

P8_bench: paralelo.c
	$(CC) $(CFLAGS) -DBENCHMARK paralelo.c -o P8_bench

clean:
	rm -f P8_mandelbrot sequencial P8_bench
