/*
 * Compilação: gcc -O3 -march=native -fopenmp paralelo.c -o P8_mandelbrot
 *
 * REGRAS DO CADERNO ("O QUE NÃO CONTA COMO SOLUÇÃO"):
 * - PROIBIDO simplesmente colocar `#pragma omp parallel for` sem analisar a
 *   política de escalonamento (`schedule`). O conjunto de Mandelbrot possui
 *   CARGA ALTAMENTE IRREGULAR: pixels no interior do cardioide/bulbo principal
 *   executam todas as `Imax` iterações (custo máximo), enquanto pixels nas
 *   bordas externas divergem em 1 ou 2 iterações (custo mínimo).
 *   O escalonamento padrão `schedule(static)` sem ajuste divide a imagem em
 *   P faixas horizontais fixas: as threads que pegam o centro da imagem ficam
 *   sobrecarregadas enquanto as threads das extremidades ficam ociosas!
 * - PROIBIDO usar `atomic` ou `critical` ao escrever em `grid[(long)y * W + x]`,
 *   pois cada par `(x, y)` mapeia para um índice único na memória (não há
 *   condição de corrida entre pixels distintos).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

/*
 * ----------------------------------------------------------------------------
 * MÓDULO 1: LEITURA DE ENTRADA (Excluída da medição de tempo)
 * ----------------------------------------------------------------------------
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
 * ----------------------------------------------------------------------------
 * MÓDULO 2: EXECUÇÃO SEQUENCIAL DE REFERÊNCIA
 * ----------------------------------------------------------------------------
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

/*
 * ----------------------------------------------------------------------------
 * MÓDULO 3: EXECUÇÃO PARALELA COM OPENMP (A SER IMPLEMENTADA PELO GRUPO)
 * ----------------------------------------------------------------------------
 * ANÁLISE DE DEPENDÊNCIAS DE DADOS:
 * 1. Laços `y` (0..H-1) e `x` (0..W-1): Cada pixel `(x, y)` é matematicamente
 *    independente de todos os outros pixels e escreve exclusivamente na posição
 *    `grid[(long)y * W + x]`. Portanto, NÃO há dependências RAW, WAR ou WAW
 *    entre diferentes pixels `(x, y)`.
 * 2. Laço interno `while (it < Imax && ...)`: A sequência `z_{n+1} = z_n^2 + c`
 *    possui dependência verdadeira (RAW) estrita da iteração `n` para `n+1`
 *    (`zr` e `zi` dependem dos valores do passo anterior). Logo, o laço `while`
 *    NÃO PODE ser paralelizado!
 * 3. Escopo de variáveis: `cr, ci, zr, zi, t, it` são declaradas dentro do
 *    corpo do laço paralelizado, sendo automaticamente PRIVADAS para cada thread.
 *    Se você usar `collapse(2)`, tanto `y` quanto `x` tornam-se privados. Se
 *    paralelizar apenas o laço externo `y`, o índice `x` do laço interno (declarado
 *    como `for (int x = 0; ...)`) também já é privado em C99/C11.
 *
 * POR QUE ESCOLHER `schedule(dynamic, chunk)` OU `schedule(guided)`?
 * - Como o custo de uma linha `y` (ou de um bloco de pixels) varia em até
 *   `Imax` vezes (10.000x!), o escalonamento dinâmico coloca blocos (chunks)
 *   em uma fila de trabalho: assim que uma thread termina uma linha "leve"
 *   (fora do conjunto), ela imediatamente pega a próxima linha disponível,
 *   evitando ociosidade (desbalanceamento de carga).
 * - CUIDADO COM O FALSO COMPARTILHAMENTO (FALSE SHARING) E OVERHEAD DE FILA:
 *   * Uma linha de cache tem 64 bytes (16 inteiros `int` de 4 bytes).
 *   * Se você paralelizar apenas o laço externo `y` (por linhas inteiras) e
 *     `W >= 16` (na medição oficial `W = 10000`), cada linha `y` ocupa
 *     `40.000` bytes, portanto threads em linhas diferentes raramente tocam na
 *     mesma linha de cache, e cada iteração `y` tem granularidade grossa o
 *     suficiente para que `schedule(dynamic, 1)` ou `schedule(guided)` tenha
 *     overhead de fila desprezível.
 *   * Se você usar `collapse(2)` (colapsando `y` e `x` em `W * H = 10^8` pixels),
 *     NUNCA use `schedule(dynamic, 1)`! Um chunk de 1 pixel causaria 10^8
 *     acessos atômicos à fila do OpenMP (overhead gigantesco) E faria 16 threads
 *     diferentes escreverem nos mesmos 64 bytes de `grid`, gerando False Sharing
 *     severo! Com `collapse(2)`, o `chunk_size` deve ser múltiplo de pelo menos
 *     64 ou 256 pixels (ex: `schedule(dynamic, 256)` ou `schedule(guided, 256)`).
 */
static void calcular_mandelbrot_paralelo(int W, int H, int Imax, int *grid) {
    /*
     * TODO 1 (Paralelização e Balanceamento de Carga Irregular):
     * Adicione abaixo a diretiva `#pragma omp parallel for` antes do laço `y`:
     * 1. Escolha e justifique a política de escalonamento para carga irregular:
     *    - Teste `schedule(static)` (para mostrar no relatório o desbalanceamento),
     *    - Teste `schedule(dynamic, chunk)` (ex: chunk = 1 ou 16 ao paralelizar por
     *      linhas `y`; ou chunk = 256/1024 se usar `collapse(2)`),
     *    - Teste `schedule(guided)` (blocos decrescentes que amortizam o overhead).
     * 2. (Dica Experimental) Use `schedule(runtime)` durante a bateria de testes
     *    para poder comparar `OMP_SCHEDULE="static"`, `OMP_SCHEDULE="dynamic,16"` e
     *    `OMP_SCHEDULE="guided"` no relatório sem precisar recompilar, ou fixe a
     *    cláusula vencedora no código final entregue!
     */

    /*
     * SOLUÇÃO DO TODO 1 (Membro 1 - laço externo y + análise static vs static,1):
     * - Paralelizamos o laço EXTERNO `y`: cada linha é uma unidade de trabalho de
     *   granularidade grossa (W pixels), o que amortiza o overhead do escalonador.
     * - `x`, `cr`, `ci`, `zr`, `zi`, `t`, `it` são declarados dentro do laço,
     *   portanto PRIVADOS por thread. `W`, `H`, `Imax` e `grid` são SHARED: os três
     *   primeiros só são lidos e cada linha `y` escreve num trecho disjunto de `grid`
     *   (sem condição de corrida, sem atomic/critical).
     * - O `while` interno NÃO é paralelizado: z_{n+1} depende de z_n (RAW estrita).
     * - `schedule(runtime)`: a política vem de OMP_SCHEDULE, permitindo comparar
     *   static, static,1, dynamic,chunk e guided sem recompilar. Sem OMP_SCHEDULE
     *   definida, o libgomp usa `dynamic,1`. No código final, trocar por
     *   a política vencedora fixa (decisão do grupo após os testes).
     *
     * SOLUÇÃO DO TODO 1 (Membro 2 - Escalonamento Dinâmico e Guiado / Ajuste de Chunk):
     * - Testamos dynamic com chunks 1, 16 e 64, além de guided:
     *   * schedule(dynamic, 1): distribui 1 linha por vez (bom balanceamento, mas mais overhead na fila).
     *   * schedule(dynamic, 16): ponto ótimo! Blocos de 16 linhas diminuem o overhead da fila em 16x
     *     e continuam pequenos o suficiente para nenhuma thread ficar parada no final.
     *   * schedule(dynamic, 64): blocos muito grandes, pode sobrar uma thread sozinha no fim.
     *   * schedule(guided): começa com blocos grandes e vai diminuindo até 1 linha no final.
     */
    /*
     * SOLUÇÃO DO TODO 1 (Membro 3 - collapse(2) e Prevenção de False Sharing):
     * - collapse(2) junta os laços `y` e `x` num único espaço de W*H iterações
     *   (pixels em vez de linhas), deixando a divisão de trabalho mais fina.
     * - Os dois laços são perfeitamente aninhados (nada entre o `for y` e o
     *   `for x`), condição exigida pelo collapse. `y` e `x` ficam privados.
     * - FALSE SHARING: uma linha de cache tem 64 bytes = 16 ints. Com collapse,
     *   pixels vizinhos podem ir para threads diferentes; se duas threads
     *   escrevem na mesma linha de cache, ela fica sendo invalidada entre os
     *   núcleos. Por isso o chunk precisa ser >= 16 pixels: usamos 256.
     * - Compilar esta versão: make P8_collapse P8_collapse_bench
     *   (sem -DUSAR_COLLAPSE o código continua com dynamic,16 por linha).
     */
#ifdef USAR_COLLAPSE
    #pragma omp parallel for collapse(2) schedule(runtime)
#else
    // #pragma omp parallel for schedule(runtime)
    // #pragma omp parallel for schedule(dynamic, 1)
    // #pragma omp parallel for schedule(dynamic, 64)
    // #pragma omp parallel for schedule(guided)
    #pragma omp parallel for schedule(dynamic, 16)
#endif
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

#ifdef PERFIL_THREADS
/*
 * Instrumentação opcional (NÃO entra no benchmark oficial):
 * mede o tempo ocupado de cada thread para evidenciar o desbalanceamento.
 * Compile: gcc -O3 -march=native -fopenmp -DPERFIL_THREADS paralelo.c -o P8_perfil
 * Rode:    OMP_NUM_THREADS=4 OMP_SCHEDULE=static ./P8_perfil < entrada.in > /dev/null
 */
static void perfil_por_thread(int W, int H, int Imax, int *grid) {
    double t_thread[256] = {0.0};
    double t_ini = omp_get_wtime();
    #pragma omp parallel
    {
        double t0 = omp_get_wtime();
        #pragma omp for schedule(runtime) nowait
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
        t_thread[omp_get_thread_num()] = omp_get_wtime() - t0;
    }
    double t_total = omp_get_wtime() - t_ini;
    int P = omp_get_max_threads();
    double t_min = t_thread[0], t_max = t_thread[0];
    for (int i = 0; i < P; i++) {
        fprintf(stderr, "[Perfil P8] thread %d ocupada por %.4f s\n", i, t_thread[i]);
        if (t_thread[i] < t_min) t_min = t_thread[i];
        if (t_thread[i] > t_max) t_max = t_thread[i];
    }
    /* Desbalanceamento = thread mais carregada / mais ociosa (1.00 = perfeito) */
    fprintf(stderr, "[Perfil P8] Tempo total = %.4f s | max/min = %.2f\n",
            t_total, (t_min > 0.0) ? t_max / t_min : 0.0);
}
#endif

/*
 * ----------------------------------------------------------------------------
 * MÓDULO 4: VALIDAÇÃO EXATA (Igualdade Byte a Byte na Grade W x H)
 * ----------------------------------------------------------------------------
 * Como cada pixel (x, y) executa a mesma sequência determinística de operações
 * IEEE 754 independentemente da thread que o processa, o número de iterações
 * `it` (inteiro) resultante em `grid[(long)y * W + x]` deve ser EXATAMENTE
 * idêntico (byte a byte) entre a versão sequencial e a paralela.
 */
static int validar_grade(const int *grid_seq, const int *grid_par, int W, int H) {
    long total = (long)W * H;
    for (long i = 0; i < total; i++) {
        if (grid_seq[i] != grid_par[i]) {
            fprintf(stderr, "[Validacao P8 FALHOU] Pixel idx=%ld (y=%ld, x=%ld) difere: seq=%d vs par=%d\n",
                    i, i / W, i % W, grid_seq[i], grid_par[i]);
            return 0;
        }
    }
    fprintf(stderr, "[Validacao P8 OK] Igualdade exata (byte a byte) nos %ld pixels.\n", total);
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

static double medir_mediana_paralela(int W, int H, int Imax, int *grid_par) {
    const int NUM_RUNS = 5;
    double tempos[5] = {0.0};

    /*
     * TODO 2 (Protocolo de Medição):
     * Execute com `OMP_NUM_THREADS` em {1, 2, 4, 6}.
     * 1. A 1ª execução abaixo é o Warm-up (descartada).
     * 2. As 5 execuções seguintes medem apenas `calcular_mandelbrot_paralelo`.
     * 3. Retorna-se a mediana dos 5 tempos.
     */
    calcular_mandelbrot_paralelo(W, H, Imax, grid_par); /* Warm-up */

    for (int r = 0; r < NUM_RUNS; r++) {
        double t0 = omp_get_wtime();
        calcular_mandelbrot_paralelo(W, H, Imax, grid_par);
        tempos[r] = omp_get_wtime() - t0;
    }

    qsort(tempos, NUM_RUNS, sizeof(double), comparar_doubles);
    return tempos[NUM_RUNS / 2];
}

int main(void) {
    int W = 0, H = 0, Imax = 0;
    if (!ler_parametros(&W, &H, &Imax)) {
        fprintf(stderr, "Erro na leitura de W, H e Imax.\n");
        return 1;
    }

#ifdef USAR_COLLAPSE
    /* Membro 3: sem OMP_SCHEDULE definida, collapse(2) usa guided,256 */
    if (getenv("OMP_SCHEDULE") == NULL) omp_set_schedule(omp_sched_guided, 256);
#endif

    long total_pixels = (long)W * H;
    int *grid_par = (int *)malloc(sizeof(int) * (size_t)total_pixels);
    if (!grid_par) {
        fprintf(stderr, "Erro na alocacao da grade paralela.\n");
        return 1;
    }

#ifdef BENCHMARK
    /*
     * Modo Benchmark + Validação:
     * Compile com: gcc -O3 -march=native -fopenmp -DBENCHMARK paralelo.c -o P8_bench
     */
    int *grid_seq = (int *)malloc(sizeof(int) * (size_t)total_pixels);
    if (!grid_seq) {
        free(grid_par);
        return 1;
    }

    double t0_seq = omp_get_wtime();
    calcular_mandelbrot_sequencial(W, H, Imax, grid_seq);
    double tempo_seq = omp_get_wtime() - t0_seq;

    double mediana_par = medir_mediana_paralela(W, H, Imax, grid_par);

    if (!validar_grade(grid_seq, grid_par, W, H)) {
        free(grid_seq);
        free(grid_par);
        return 2;
    }

    int P = omp_get_max_threads();
    fprintf(stderr, "[Benchmark P8] Threads (P) = %d | T_seq = %.6f s | T_par (mediana) = %.6f s\n",
            P, tempo_seq, mediana_par);
    free(grid_seq);
#elif defined(PERFIL_THREADS)
    perfil_por_thread(W, H, Imax, grid_par);
#else
    /* Modo padrão compatível com ./test.sh */
    double t0 = omp_get_wtime();
    calcular_mandelbrot_paralelo(W, H, Imax, grid_par);
    double dt = omp_get_wtime() - t0;
    fprintf(stderr, "[Paralelo P8] Threads = %d | Tempo = %.6f s\n",
            omp_get_max_threads(), dt);
#endif

    for (long i = 0; i < total_pixels; i++) {
        printf("%d\n", grid_par[i]);
    }

    free(grid_par);
    return 0;
}