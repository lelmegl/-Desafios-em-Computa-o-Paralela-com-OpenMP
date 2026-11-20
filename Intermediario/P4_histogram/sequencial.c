/*

 * Compilação: gcc -O3 -march=native -fopenmp sequencial.c -o sequencial
 *
 * NOTA OBRIGATÓRIA SOBRE O REBASE DO SEQUENCIAL (Documentar na Seção 2 do Relatório):
 * O código base original do caderno realizava chamadas de `scanf("%d", &v)`
 * DENTRO do laço de contagem do histograma (`for (long i = 0; i < M; i++)`).
 * Como `scanf` possui bloqueio de stream (flockfile/mutex interno da libc) e é
 * estritamente serial e limitado por I/O, mantê-lo dentro da região medida
 * violaria a metodologia de medição do caderno (que exige medir apenas a região
 * computacional) e impediria a paralelização do cálculo em memória.
 * Portanto, separamos a leitura dos M pixels para um vetor alocado dinamicamente
 * (`unsigned char *pixels`) ANTES da região computacional medida com `omp_get_wtime()`.
 * Conforme as "Regras do Jogo" (pág. 3 e 4 do caderno), esta alteração deve ser
 * declarada e justificada no relatório, e o Speedup S(P) = T(1)/T(P) deve ser
 * recalculado contra esta nova versão sequencial em memória!
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define NUM_BINS 256

/*
 * 1. Módulo de Entrada e Alocação (Excluído da medição de tempo)
 * Armazena os pixels em `unsigned char` (1 byte por pixel) para que M = 10^9
 * ocupe ~1 GB de RAM (em vez de 4 GB com `int`), melhorando também a largura
 * de banda de memória e a localidade espacial no cache L1/L2.
 */
static unsigned char *ler_pixels(long *M_out) {
    long M = 0;
    if (scanf("%ld", &M) != 1 || M <= 0) {
        return NULL;
    }

    unsigned char *pixels = (unsigned char *)malloc((size_t)M * sizeof(unsigned char));
    if (!pixels) {
        return NULL;
    }

    for (long i = 0; i < M; i++) {
        int v;
        if (scanf("%d", &v) != 1 || v < 0 || v > 255) {
            free(pixels);
            return NULL;
        }
        pixels[i] = (unsigned char)v;
    }

    *M_out = M;
    return pixels;
}

/*
 * 2. Kernel Computacional Sequencial (Baseline Rebaseado)
 * Percorre o vetor de M pixels em memória e acumula a frequência nos 256 bins.
 */
static void calcular_histograma_sequencial(const unsigned char *pixels, long M, long histo[NUM_BINS]) {
    memset(histo, 0, NUM_BINS * sizeof(long));

    for (long i = 0; i < M; i++) {
        histo[pixels[i]]++;
    }
}

int main(void) {
    long M = 0;
    unsigned char *pixels = ler_pixels(&M);
    if (!pixels) {
        fprintf(stderr, "Erro na leitura ou alocacao dos pixels.\n");
        return 1;
    }

    long histo[NUM_BINS] = {0};

    /* Medição de tempo estritamente ao redor do cálculo do histograma em memória */
    double t0 = omp_get_wtime();
    calcular_histograma_sequencial(pixels, M, histo);
    double dt = omp_get_wtime() - t0;

    /* Saída padrão exigida: 256 linhas, uma para cada bin */
    for (int b = 0; b < NUM_BINS; b++) {
        printf("%ld\n", histo[b]);
    }

    fprintf(stderr, "[Sequencial Rebaseado P4] M = %ld | Tempo = %.6f s\n", M, dt);

    free(pixels);
    return 0;
}
