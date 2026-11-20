/*


 * Compilação: gcc -O3 -march=native -fopenmp sequencial.c -o sequencial
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

/*
 * 1. Módulo de Entrada (Excluído da medição de tempo)
 */
static int ler_parametros(int *W, int *H, int *Imax) {
    if (scanf("%d %d %d", W, H, Imax) != 3) {
        return 0;
    }
    if (*W <= 0 || *H <= 0 || *Imax <= 0) {
        return 0;
    }
    return 1;
}

/*
 * 2. Kernel Computacional Sequencial
 * Para cada pixel (x, y) em uma grade W x H mapeada em [-2, 1] x [-1.5, 1.5],
 * itera z_{n+1} = z_n^2 + c partindo de z_0 = 0 até |z|^2 > 4.0 ou Imax.
 */
static void calcular_mandelbrot_sequencial(int W, int H, int Imax, int *grid) {
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            double cr = -2.0 + 3.0 * x / W;
            double ci = -1.5 + 3.0 * y / H;
            double zr = 0.0, zi = 0.0;
            int it = 0;

            while (it < Imax && zr * zr + zi * zi <= 4.0) {
                double t = zr * zr - zi * zi + cr;
                zi = 2.0 * zr * zi + ci;
                zr = t;
                it++;
            }

            grid[(long)y * W + x] = it;
        }
    }
}

int main(void) {
    int W = 0, H = 0, Imax = 0;
    if (!ler_parametros(&W, &H, &Imax)) {
        fprintf(stderr, "Erro na leitura de W, H e Imax.\n");
        return 1;
    }

    long total_pixels = (long)W * H;
    int *grid = (int *)malloc(sizeof(int) * (size_t)total_pixels);
    if (!grid) {
        fprintf(stderr, "Erro na alocacao da grade.\n");
        return 1;
    }

    /* Medição de tempo estritamente ao redor do cálculo do fractal */
    double t0 = omp_get_wtime();
    calcular_mandelbrot_sequencial(W, H, Imax, grid);
    double dt = omp_get_wtime() - t0;

    /* Saída padrão exigida: W * H inteiros em row-major, um por linha */
    for (long i = 0; i < total_pixels; i++) {
        printf("%d\n", grid[i]);
    }

    fprintf(stderr, "[Sequencial P8] %dx%d (Imax=%d) | Tempo = %.6f s\n", W, H, Imax, dt);

    free(grid);
    return 0;
}
