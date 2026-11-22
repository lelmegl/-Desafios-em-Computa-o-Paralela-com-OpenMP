CC = gcc
CFLAGS = -O3 -march=native -fopenmp -Wall -Wextra

all: P8_mandelbrot sequencial P8_bench P8_perfil P8_collapse P8_collapse_bench

P8_mandelbrot: paralelo.c
	$(CC) $(CFLAGS) paralelo.c -o P8_mandelbrot

sequencial: sequencial.c
	$(CC) $(CFLAGS) sequencial.c -o sequencial

P8_bench: paralelo.c
	$(CC) $(CFLAGS) -DBENCHMARK paralelo.c -o P8_bench

P8_perfil: paralelo.c
	$(CC) $(CFLAGS) -DPERFIL_THREADS paralelo.c -o P8_perfil

P8_collapse: paralelo.c
	$(CC) $(CFLAGS) -DUSAR_COLLAPSE paralelo.c -o P8_collapse

P8_collapse_bench: paralelo.c
	$(CC) $(CFLAGS) -DUSAR_COLLAPSE -DBENCHMARK paralelo.c -o P8_collapse_bench

clean:
	rm -f P8_mandelbrot sequencial P8_bench P8_perfil P8_collapse P8_collapse_bench
