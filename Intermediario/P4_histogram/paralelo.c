/*
 * Compilação: gcc -O3 -march=native -fopenmp paralelo.c -o P4_histogram
 *
 * REGRAS DO CADERNO ("O QUE NÃO CONTA COMO SOLUÇÃO"):
 * - PROIBIDO usar `#pragma omp atomic` ou `#pragma omp critical` em toda escrita
 *   `histo[v]++`. São apenas 256 bins para até 10^9 escritas: todas as threads
 *   disputariam as mesmas 256 posições a cada ciclo de clock, causando contenção
 *   severa de coerência de cache (ping-pong de linhas de cache MESI) e gerando
 *   SPEEDUP NEGATIVO (o programa paralelo fica muito mais lento que o sequencial!).
 * - OBRIGATÓRIO documentar no relatório (Seção 2 - Estratégia) a separação da
 *   leitura `scanf` para fora do laço medido e recalcular o speedup (rebase)
 *   contra este novo sequencial em memória.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define NUM_BINS 256

/*
 * ----------------------------------------------------------------------------
 * MÓDULO 1: LEITURA DE ENTRADA EM MEMÓRIA (Separada da Região Paralela)
 * ----------------------------------------------------------------------------
 * IMPORTANTE PARA O RELATÓRIO:
 * No código original do caderno, `scanf("%d", &v)` estava dentro do laço `for`.
 * Para permitir paralelismo real de dados e medir apenas o kernel computacional
 * (conforme a Seção 1.6 do caderno), os M pixels são lidos previamente para um
 * vetor alocado dinamicamente no heap. Documente esta modificação no Capítulo 2
 * do relatório para justificar o rebase do tempo sequencial T(1).
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
 * ----------------------------------------------------------------------------
 * MÓDULO 2: EXECUÇÃO SEQUENCIAL DE REFERÊNCIA (REBASEADA)
 * ----------------------------------------------------------------------------
 */
static void calcular_histograma_sequencial(const unsigned char *pixels, long M, long histo[NUM_BINS]) {
    memset(histo, 0, NUM_BINS * sizeof(long));

    for (long i = 0; i < M; i++) {
        histo[pixels[i]]++;
    }
}

/*
 * ----------------------------------------------------------------------------
 * MÓDULO 3: EXECUÇÃO PARALELA COM OPENMP (A SER IMPLEMENTADA PELO GRUPO)
 * ----------------------------------------------------------------------------
 * ANÁLISE DE DEPENDÊNCIAS DE DADOS E CONTENÇÃO EM MEMÓRIA:
 * 1. Vetor `pixels`: Apenas leitura (`pixels[i]`). Não há dependência entre as
 *    leituras de diferentes índices `i`. Pode ser `shared` com acesso contíguo
 *    (excelente localidade espacial para `schedule(static)`).
 * 2. Vetor `histo[256]`: A instrução `histo[v]++` lê o valor antigo de `histo[v]`,
 *    soma 1 e escreve de volta em `histo[v]`. Como diferentes iterações `i1` e `i2`
 *    frequentemente possuem a mesma cor `v = pixels[i1] = pixels[i2]`, existe
 *    dependência verdadeira (RAW), antidependência (WAR) e dependência de saída
 *    (WAW) na posição `histo[v]`.
 *
 * COMO ELIMINAR A CONTENÇÃO E O FALSO COMPARTILHAMENTO (FALSE SHARING)?
 * Existem duas abordagens legítimas e de alto desempenho em OpenMP (o grupo pode
 * implementar uma ou testar ambas para comparar no relatório):
 *
 * ABORDAGEM A (Privatização Manual de Histogramas por Thread + Padding):
 * - Abre-se uma região `#pragma omp parallel`.
 * - Cada thread declara seu próprio histograma privado na própria pilha (stack),
 *   por exemplo: `long local_histo[NUM_BINS] = {0};` (ou aloca uma matriz 2D
 *   alinhada `alignas(64)` / com padding para múltiplo de 64 bytes, que é o
 *   tamanho da linha de cache L1, garantindo que duas threads nunca escrevam
 *   na mesma linha de cache — evitando False Sharing!).
 * - Com `#pragma omp for schedule(static) nowait`, cada thread conta os pixels
 *   do seu bloco diretamente em `local_histo[pixels[i]]++` em velocidade máxima
 *   de cache L1 privado, SEM nenhuma sincronização durante os M passos!
 * - Ao sair do laço de M passos, reduz-se `local_histo` no vetor global `histo`
 *   (apenas 256 somas por thread, onde um `#pragma omp critical` ou laço de
 *   redução final executa apenas P * 256 operações, custo desprezível O(P * 256)).
 *
 * ABORDAGEM B (Redução Nativa em Array do OpenMP 4.5+):
 * - Usa-se `#pragma omp parallel for reduction(+:histo[:NUM_BINS]) schedule(static)`.
 * - O próprio compilador GCC cria cópias privadas do array `histo[0..255]` na
 *   pilha de cada thread e combina as 256 posições ao final da região paralela.
 */
static void calcular_histograma_paralelo(const unsigned char *pixels, long M, long histo[NUM_BINS]) {
    memset(histo, 0, NUM_BINS * sizeof(long));

    /*
     * TODO 1 (Paralelização Livre de Contenção e False Sharing):
     * Substitua o laço sequencial abaixo por uma das abordagens paralelas descritas:
     *
     * -> Se escolher a Abordagem A (Histograma Privado por Thread):
     *    1. Inicie `#pragma omp parallel`
     *    2. Crie um vetor privado `long local_histo[NUM_BINS] = {0};` dentro do bloco
     *       paralelo (ou use ` __attribute__((aligned(64))) ` para evitar False Sharing).
     *    3. Divida o laço de `0` a `M` com `#pragma omp for schedule(static) nowait`,
     *       incrementando `local_histo[pixels[i]]++`.
     *    4. Após o laço `for` de `M` elementos, some `local_histo[b]` em `histo[b]`
     *       para `b` de `0` a `255` de forma segura (aqui sim um bloco `#pragma omp critical`
     *       ao redor do pequeno laço de 256 posições é válido e eficiente, pois roda
     *       apenas 1 vez por thread ao final, e não M vezes!).
     *
     * -> Se escolher a Abordagem B (Reduction em Array OpenMP 4.5+):
     *    1. Aplique `#pragma omp parallel for reduction(+:histo[:NUM_BINS]) schedule(static)`
     *       diretamente sobre o laço `for (long i = 0; i < M; i++)`.
     *    2. Dica para o relatório: meça e compare as Abordagens A e B!
     */
    for (long i = 0; i < M; i++) {
        histo[pixels[i]]++;
    }
}

/*
 * ----------------------------------------------------------------------------
 * MÓDULO 4: VALIDAÇÃO EXATA (Igualdade Byte a Byte para Inteiros)
 * ----------------------------------------------------------------------------
 * Regra do Caderno (Prefácio e Seção 1.4): Para problemas inteiros, a saída
 * paralela deve coincidir EXATAMENTE (byte a byte) com a saída sequencial.
 * Além disso, a soma de todos os 256 bins deve ser exatamente igual a M.
 */
static int validar_histograma(const long histo_seq[NUM_BINS], const long histo_par[NUM_BINS], long M) {
    long soma_par = 0;
    for (int b = 0; b < NUM_BINS; b++) {
        soma_par += histo_par[b];
        if (histo_seq[b] != histo_par[b]) {
            fprintf(stderr, "[Validacao P4 FALHOU] Bin %d difere: seq = %ld vs par = %ld\n",
                    b, histo_seq[b], histo_par[b]);
            return 0;
        }
    }
    if (soma_par != M) {
        fprintf(stderr, "[Validacao P4 FALHOU] Soma dos bins (%ld) != M (%ld)\n", soma_par, M);
        return 0;
    }
    fprintf(stderr, "[Validacao P4 OK] Igualdade exata confirmada nos 256 bins (Total = %ld).\n", soma_par);
    return 1;
}

/*
 * ----------------------------------------------------------------------------
 * MÓDULO 5: PROTOCOLO DE MEDIÇÃO (Warm-up + Mediana de 5 Execuções)
 * ----------------------------------------------------------------------------
 */
static int comparar_doubles(const void *a, const void *b) {
    double da = *(const double *)a;
    double db = *(const double *)b;
    return (da > db) - (da < db);
}

static double medir_mediana_paralela(const unsigned char *pixels, long M, long histo_par[NUM_BINS]) {
    const int NUM_RUNS = 5;
    double tempos[5] = {0.0};

    /*
     * TODO 2 (Protocolo de Medição):
     * Certifique-se de rodar com `OMP_NUM_THREADS` em {1, 2, 4, 6}.
     * 1. A 1ª chamada abaixo atua como Warm-up (descartada).
     * 2. As 5 chamadas seguintes são cronometradas exclusivamente ao redor
     *    de `calcular_histograma_paralelo` (sem I/O).
     * 3. Retorna-se a mediana (`tempos[2]` após ordenação).
     */
    calcular_histograma_paralelo(pixels, M, histo_par); /* Warm-up */

    for (int r = 0; r < NUM_RUNS; r++) {
        double t0 = omp_get_wtime();
        calcular_histograma_paralelo(pixels, M, histo_par);
        tempos[r] = omp_get_wtime() - t0;
    }

    qsort(tempos, NUM_RUNS, sizeof(double), comparar_doubles);
    return tempos[NUM_RUNS / 2];
}

int main(void) {
    long M = 0;
    unsigned char *pixels = ler_pixels(&M);
    if (!pixels) {
        fprintf(stderr, "Erro na leitura da entrada.\n");
        return 1;
    }

    long histo_par[NUM_BINS] = {0};

#ifdef BENCHMARK
    /*
     * Modo Benchmark + Validação:
     * Compile com: gcc -O3 -march=native -fopenmp -DBENCHMARK paralelo.c -o P4_bench
     */
    long histo_seq[NUM_BINS] = {0};
    double t0_seq = omp_get_wtime();
    calcular_histograma_sequencial(pixels, M, histo_seq);
    double tempo_seq = omp_get_wtime() - t0_seq;

    double mediana_par = medir_mediana_paralela(pixels, M, histo_par);

    if (!validar_histograma(histo_seq, histo_par, M)) {
        free(pixels);
        return 2;
    }

    int P = omp_get_max_threads();
    fprintf(stderr, "[Benchmark P4] Threads (P) = %d | T_seq = %.6f s | T_par (mediana) = %.6f s\n",
            P, tempo_seq, mediana_par);
#else
    /* Modo padrão compatível com ./test.sh */
    double t0 = omp_get_wtime();
    calcular_histograma_paralelo(pixels, M, histo_par);
    double dt = omp_get_wtime() - t0;
    fprintf(stderr, "[Paralelo P4] Threads = %d | Tempo = %.6f s\n",
            omp_get_max_threads(), dt);
#endif

    for (int b = 0; b < NUM_BINS; b++) {
        printf("%ld\n", histo_par[b]);
    }

    free(pixels);
    return 0;
}
