/*

 * Compilação: gcc -O3 -march=native -fopenmp paralelo.c -o P1_pi -lm
 *
 * REGRAS DO CADERNO ("O QUE NÃO CONTA COMO SOLUÇÃO"):
 * - PROIBIDO usar #pragma omp atomic ou #pragma omp critical dentro do laço
 *   de integração (gera contenção severa de hardware/cache e speedup negativo).
 * - PROIBIDO usar #pragma omp parallel sem dividir o trabalho com 'for'.
 * - PROIBIDO fixar threads no código com omp_set_num_threads() para burlar a
 *   variável de ambiente OMP_NUM_THREADS durante os testes oficiais.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

/*
 * ----------------------------------------------------------------------------
 * MÓDULO 1: LEITURA DE ENTRADA (Excluída da medição de tempo)
 * ----------------------------------------------------------------------------
 */
static int ler_entrada(long long *N) {
    if (scanf("%lld", N) != 1 || *N <= 0) {
        return 0;
    }
    return 1;
}

/*
 * ----------------------------------------------------------------------------
 * MÓDULO 2: EXECUÇÃO SEQUENCIAL DE REFERÊNCIA
 * ----------------------------------------------------------------------------
 * Mantida para validação interna de corretude (|Delta| <= 10^-9) e comparação
 * do efeito de reassociação em ponto flutuante (IEEE 754).
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

/*
 * ----------------------------------------------------------------------------
 * MÓDULO 3: EXECUÇÃO PARALELA COM OPENMP (A SER IMPLEMENTADA PELO GRUPO)
 * ----------------------------------------------------------------------------
 * ANÁLISE DE DEPENDÊNCIAS DE DADOS E ESCOPO DE VARIÁVEIS:
 * 1. Variável 'x': Calculada e consumida na mesma iteração. Declarada dentro
 *    do escopo do laço, torna-se naturalmente PRIVADA por thread, evitando
 *    dependências anti (WAR - Write-After-Read) e de saída (WAW - Write-After-Write).
 * 2. Variáveis 'N' e 'step': Apenas lidas por todas as threads (Read-Only).
 *    Podem permanecer COMPARTILHADAS (shared) com total segurança.
 * 3. Variável 'sum': Sofre acumulação `sum = sum + termo` a cada iteração.
 *    Isto caracteriza uma dependência verdadeira (RAW - Read-After-Write) e WAW
 *    entre iterações (loop-carried dependence). Se múltiplas threads escreverem
 *    em 'sum' compartilhada sem proteção, ocorrerá Condição de Corrida (Race Condition).
 *
 * POR QUE ESCOLHER 'reduction(+:sum)' E 'schedule(static)'?
 * - A cláusula `reduction(+:sum)` instrui o compilador a criar uma cópia privada
 *   de `sum` para cada thread (inicializada com 0.0). Cada thread acumula sua
 *   soma parcial localmente nos registradores da CPU sem nenhuma disputa de
 *   barramento ou invalidação de linha de cache (Zero False Sharing). Apenas no
 *   Join (final do laço), as P somas parciais são combinadas de forma segura.
 * - Como cada iteração executa exatamente as mesmas instruções aritméticas
 *   (carga 100% uniforme), o escalonamento `schedule(static)` divide as N
 *   iterações em P blocos contíguos iguais no início, com overhead mínimo.
 */
static double calcular_pi_paralelo(long long N) {
    double step = 1.0 / (double)N;
    double sum = 0.0;

    /*
     * TODO 1 (Paralelização do Laço e Redução):
     * Adicione abaixo a diretiva OpenMP para paralelizar o laço 'for':
     * - Use `#pragma omp parallel for`
     * - Adicione a cláusula `reduction(+:sum)` para tratar a dependência em 'sum'.
     * - (Opcional/Recomendado) Explicite `schedule(static)` e verifique os escopos
     *   (lembre-se que o índice `i` do laço e `x` declarado internamente já são
     *   privados por padrão).
     * - Experimento extra para o relatório: teste também combinar com `simd`
     *   (`#pragma omp parallel for simd reduction(+:sum)`) e analise se o
     *   compilador com `-O3 -march=native` já vetorizava o laço.
     */

    /*
     * SOLUÇÃO DO TODO 1 (Membro 2 - Variante SIMD e Reassociação IEEE 754):
     * - Compilando com -DP1_SIMD (alvo `make P1_bench_simd`), o laço usa
     *   `parallel for simd`: além de dividir as N iterações entre as threads,
     *   cada thread processa seu bloco em vetores AVX2 (4 doubles por instrução).
     * - Sem `simd`, o `-O3 -march=native` NÃO vetoriza este laço: vetorizar uma
     *   soma em double exige reordenar as parcelas, e o padrão IEEE 754 proíbe o
     *   compilador de fazer isso sozinho (só com -ffast-math). A cláusula `simd`
     *   dá essa permissão de forma localizada, só neste laço.
     * - Efeito colateral: a soma passa a ser feita em 4 "pistas" por thread e
     *   depois combinada, ou seja, outra ordem de soma. Como (a+b)+c != a+(b+c)
     *   em ponto flutuante, pi_par difere de pi_seq lá pelo 13º-15º dígito.
     *   A diferença fica muito abaixo da tolerância |Delta_rel| <= 1e-9.
     */
    /*
     * SOLUÇÃO DO TODO 1 (Membro 1 - Kernel Paralelo Base e Escopo):
     * - `parallel for`: divide as N iterações do laço entre as P threads.
     * - `reduction(+:sum)`: cada thread recebe uma cópia PRIVADA de `sum`
     *   (iniciada em 0.0, elemento neutro da soma) e acumula só nela. No fim do
     *   laço o OpenMP soma as P cópias na `sum` original. Assim nenhuma thread
     *   escreve na mesma variável que outra: sem condição de corrida e sem
     *   false sharing (as cópias não ficam num vetor compartilhado).
     * - Sem o reduction, `sum += ...` em `sum` compartilhada perderia parcelas
     *   (ler-somar-escrever de duas threads ao mesmo tempo) e pi sairia errado.
     *   Usar `atomic` resolveria a corrida, mas seriam N = 10^9 operações
     *   atômicas na mesma variável: mais lento que o sequencial.
     * - `schedule(static)`: toda iteração tem o mesmo custo, então dividir em P
     *   blocos contíguos iguais já balanceia a carga, sem custo de fila.
     * - Escopo: `i` (índice do omp for) e `x` (declarada dentro do laço) são
     *   PRIVADAS; `N` e `step` são SHARED (só leitura); `sum` é REDUCTION.
     */
#if defined(P1_SIMD)
    #pragma omp parallel for simd reduction(+:sum) schedule(static)
#else
    #pragma omp parallel for reduction(+:sum) schedule(static)
#endif
    for (long long i = 0; i < N; i++) {
        double x = (i + 0.5) * step;
        sum += 4.0 / (1.0 + x * x);
    }

    return step * sum;
}

/*
 * ----------------------------------------------------------------------------
 * MÓDULO 4: VALIDAÇÃO NUMÉRICA (Tolerância Relativa <= 10^-9)
 * ----------------------------------------------------------------------------
 * Em aritmética de ponto flutuante IEEE 754 (double), a adição NÃO é associativa:
 * (a + b) + c != a + (b + c) devido a arredondamentos no último bit (ULP).
 * A redução paralela soma blocos em árvores/ordens diferentes da soma sequencial,
 * gerando uma divergência esperada na casa de ~10^-13 a 10^-15.
 * Por isso, a regra do caderno exige tolerância relativa |Delta| <= 10^-9.
 */
static int validar_resultado(double pi_seq, double pi_par) {
    double diff_abs = fabs(pi_par - pi_seq);
    double erro_rel = (fabs(pi_seq) > 0.0) ? (diff_abs / fabs(pi_seq)) : diff_abs;

    fprintf(stderr, "[Validacao P1] pi_seq = %.15f | pi_par = %.15f\n", pi_seq, pi_par);
    fprintf(stderr, "[Validacao P1] |Delta_abs| = %.3e | |Delta_rel| = %.3e (Limite: 1.000e-09)\n",
            diff_abs, erro_rel);

    return (erro_rel <= 1e-9);
}

/*
 * ----------------------------------------------------------------------------
 * MÓDULO 5: PROTOCOLO DE MEDIÇÃO E BENCHMARK (Mediana de 5 + Warm-up)
 * ----------------------------------------------------------------------------
 * Regras do Jogo (Seção 1.6 e Apêndice B):
 * 1. Respeitar OMP_NUM_THREADS definido no ambiente (testar P em {1, 2, 4, 6}).
 * 2. Descartar a 1ª execução (warm-up da equipe de threads, páginas e caches).
 * 3. Executar 5 vezes cronometradas com omp_get_wtime() apenas no cálculo.
 * 4. Ordenar os 5 tempos e reportar a MEDIANA (índice 2).
 */
static int comparar_doubles(const void *a, const void *b) {
    double da = *(const double *)a;
    double db = *(const double *)b;
    return (da > db) - (da < db);
}

static double medir_mediana_paralela(long long N, double *pi_out) {
    const int NUM_RUNS = 5;
    double tempos[5] = {0.0};

    /*
     * TODO 2 (Protocolo de Medição Reprodutível):
     * Quando ativar o modo BENCHMARK (compilando com -DBENCHMARK), complete ou
     * utilize esta função para coletar os tempos oficiais da tabela do relatório:
     * Passo A: Chame `calcular_pi_paralelo(N)` 1 vez fora do cronômetro (Warm-up).
     * Passo B: Em um laço de r = 0 até NUM_RUNS - 1:
     *          - Marque `double t0 = omp_get_wtime();`
     *          - Execute `*pi_out = calcular_pi_paralelo(N);`
     *          - Marque `tempos[r] = omp_get_wtime() - t0;`
     * Passo C: Ordene o vetor `tempos` com `qsort` e retorne `tempos[NUM_RUNS / 2]`.
     */
    *pi_out = calcular_pi_paralelo(N); /* Warm-up */

    for (int r = 0; r < NUM_RUNS; r++) {
        double t0 = omp_get_wtime();
        *pi_out = calcular_pi_paralelo(N);
        tempos[r] = omp_get_wtime() - t0;
    }

    qsort(tempos, NUM_RUNS, sizeof(double), comparar_doubles);
    return tempos[NUM_RUNS / 2];
}

int main(void) {
    long long N = 0;

    /* 1. Leitura fora da medição de tempo */
    if (!ler_entrada(&N)) {
        fprintf(stderr, "Erro na leitura de N.\n");
        return 1;
    }

#ifdef BENCHMARK
    /*
     * Modo completo de Benchmark + Validação para construção da Tabela e Gráficos:
     * Compile com: gcc -O3 -march=native -fopenmp -DBENCHMARK paralelo.c -o P1_bench -lm
     * Execute com: OMP_NUM_THREADS=1 ./P1_bench < pi.in (repita para 2, 4, 6)
     */
    double t0_seq = omp_get_wtime();
    double pi_seq = calcular_pi_sequencial(N);
    double tempo_seq = omp_get_wtime() - t0_seq;

    double pi_par = 0.0;
    double mediana_par = medir_mediana_paralela(N, &pi_par);

    if (!validar_resultado(pi_seq, pi_par)) {
        fprintf(stderr, "ERRO: Validacao falhou! Verifique a implementacao paralela.\n");
        return 2;
    }

    int P = omp_get_max_threads();
    fprintf(stderr, "[Benchmark P1] Threads (P) = %d | T_seq = %.6f s | T_par (mediana) = %.6f s\n",
            P, tempo_seq, mediana_par);
    printf("%.15f\n", pi_par);
#else
    /*
     * Modo padrão compatível com ./test.sh (executa apenas o kernel paralelo
     * cronometrado uma vez e imprime somente a saída esperada em stdout).
     */
    double t0 = omp_get_wtime();
    double pi_par = calcular_pi_paralelo(N);
    double dt = omp_get_wtime() - t0;

    printf("%.15f\n", pi_par);
    fprintf(stderr, "[Paralelo P1] Threads = %d | Tempo = %.6f s\n",
            omp_get_max_threads(), dt);
#endif

    return 0;
}