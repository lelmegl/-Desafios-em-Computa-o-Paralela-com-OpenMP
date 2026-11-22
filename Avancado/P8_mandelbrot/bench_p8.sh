#!/usr/bin/env bash
# Benchmark oficial do P8.
#  Parte 1: comparacao de politicas de schedule com P = 6 (binario P8_perfil, schedule(runtime)):
#           tempo total + desbalanceamento max/min entre threads -> comparacao_schedules_p8.csv
#  Parte 2: tabela oficial P em {1,2,4,6} com a politica fixada no codigo (P8_bench)
#           -> resultados_p8.csv + graficos_p8.png (validacao byte a byte em cada P)
#  Parte 1b: variante collapse(2) do Membro 3 (P8_collapse_bench) com dynamic,256 e guided,256
# Uso (dentro de Avancado/P8_mandelbrot):  ./bench_p8.sh [W H Imax]   (padrao 4000 4000 10000)
set -euo pipefail
cd "$(dirname "$0")"
W="${1:-4000}"; H="${2:-4000}"; IMAX="${3:-10000}"
ENTRADA="entrada_grande.in"
echo "$W $H $IMAX" > "$ENTRADA"
make -s P8_bench P8_perfil P8_collapse_bench
export OMP_PROC_BIND=close OMP_PLACES=cores

echo "=== Parte 1: politicas de schedule (P = 6, entrada $W x $H, Imax = $IMAX) ==="
echo "schedule,T_total_s,max_min" > comparacao_schedules_p8.csv
for S in "static" "static,1" "dynamic,1" "dynamic,16" "dynamic,64" "guided"; do
    LOG=$(OMP_NUM_THREADS=6 OMP_SCHEDULE="$S" ./P8_perfil < "$ENTRADA" 2>&1 >/dev/null)
    LINHA=$(echo "$LOG" | grep 'Tempo total')
    printf '%-11s %s\n' "$S" "$LINHA"
    T=$(echo "$LINHA" | sed -n 's/.*Tempo total = \([0-9.]*\).*/\1/p')
    R=$(echo "$LINHA" | sed -n 's/.*max\/min = \([0-9.]*\).*/\1/p')
    echo "\"$S\",$T,$R" >> comparacao_schedules_p8.csv
done

echo; echo "=== Parte 1b: collapse(2) com chunk 256 (P = 6, mediana de 5) ==="
for S in "dynamic,256" "guided,256"; do
    LOG=$(OMP_NUM_THREADS=6 OMP_SCHEDULE="$S" ./P8_collapse_bench < "$ENTRADA" 2>&1 >/dev/null)
    echo "$LOG" | grep -q 'Validacao P8 OK' || { echo "ERRO: validacao collapse falhou ($S)"; exit 1; }
    T=$(echo "$LOG" | sed -n 's/.*T_par (mediana) = \([0-9.]*\).*/\1/p')
    printf 'collapse(2) %-11s Tempo (mediana) = %s s\n' "$S" "$T"
    echo "\"collapse(2) $S\",$T," >> comparacao_schedules_p8.csv
done

echo; echo "=== Parte 2: tabela oficial (politica fixada no paralelo.c) ==="
echo "P,T_par_s,T_seq_s" > resultados_p8.csv
for P in 1 2 4 6; do
    echo ">> P = $P"
    LOG=$(OMP_NUM_THREADS=$P ./P8_bench < "$ENTRADA" 2>&1 >/dev/null)
    echo "$LOG" | grep -E 'Validacao|Benchmark'
    echo "$LOG" | grep -q 'Validacao P8 OK' || { echo "ERRO: validacao falhou em P=$P"; exit 1; }
    TPAR=$(echo "$LOG" | sed -n 's/.*T_par (mediana) = \([0-9.]*\).*/\1/p')
    TSEQ=$(echo "$LOG" | sed -n 's/.*T_seq = \([0-9.]*\).*/\1/p')
    echo "$P,$TPAR,$TSEQ" >> resultados_p8.csv
done
python3 ../../scripts/plot_speedup.py resultados_p8.csv "P8 - Mandelbrot" graficos_p8.png
