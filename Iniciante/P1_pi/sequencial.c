/*
 * Compilação: gcc -O3 -march=native -fopenmp sequencial.c -o sequencial


#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

/*
 * 1. Módulo de Entrada (I/O)
 * Isolado da medição de tempo para não contaminar a análise da Lei de Amdahl.
 */
static int ler_entrada(long long *N) {
    if (scanf("%lld", N) != 1 || *N <= 0) {
        return 0;
    }
    return 1;
}

/*
 * 2. Kernel Computacional Sequencial
 * Calcula a aproximação de pi pelo método dos retângulos no intervalo [0, 1]
 * para f(x) = 4 / (1 + x^2) com N passos.
 * Complexidade: O(N) em tempo, O(1) em espaço (CPU-bound puro).
 */
static double calcular_pi_sequencial(long long N) {
    double step = 1.0 / (double)N;
    double sum = 0.0;

    for (long long i = 0; i < N; i++) {
        double x = (i + 0.5) * step;
        sum += 4.0 / (1.0 + x * x);
    }

    return step * sum;
}

int main(void) {
    long long N = 0;
    if (!ler_entrada(&N)) {
        fprintf(stderr, "Erro: entrada N invalida.\n");
        return 1;
    }

    /* Medição de tempo de parede apenas ao redor da região computacional */
    double t0 = omp_get_wtime();
    double pi = calcular_pi_sequencial(N);
    double dt = omp_get_wtime() - t0;

    /* Saída padrão exigida pelo script de validação (15 casas decimais) */
    printf("%.15f\n", pi);

    /* Tempo impresso em stderr para não interferir no diff do test.sh */
    fprintf(stderr, "[Sequencial] N = %lld | Tempo = %.6f s\n", N, dt);

    return 0;
}
