#!/usr/bin/env bash
# Benchmark oficial do P4: gera a entrada de estresse e roda, para P em {1,2,4,6}:
#   A      = P4_bench        (histograma privado + aligned(64))   -> resultados_p4.csv
#   B      = P4_bench_B      (reduction(+:histo[:256]))          -> resultados_p4_B.csv
#   atomic = P4_bench_atomic (anti-exemplo, prova do "proibido")  -> resultados_p4_atomic.csv
# Valida byte a byte em cada rodada e gera os graficos de A e B.
# Uso (dentro de Intermediario/P4_histogram):  ./bench_p4.sh [M]   (padrao M = 50000000)
set -euo pipefail
cd "$(dirname "$0")"
M="${1:-50000000}"
ENTRADA="entrada_grande.in"   # fora do git (~175 MB para M = 5e7)

if [ ! -f "$ENTRADA" ] || [ "$(head -1 "$ENTRADA")" != "$M" ]; then
    echo ">> Gerando $ENTRADA com M = $M pixels..."
    python3 ../../gen_inputs.py p4 "$M" > "$ENTRADA"
fi
make -s P4_bench P4_bench_B P4_bench_atomic

for VAR in A B atomic; do
    case $VAR in
        A) BIN=P4_bench; CSV=resultados_p4.csv ;;
        B) BIN=P4_bench_B; CSV=resultados_p4_B.csv ;;
        atomic) BIN=P4_bench_atomic; CSV=resultados_p4_atomic.csv ;;
    esac
    echo "P,T_par_s,T_seq_s" > "$CSV"
    for P in 1 2 4 6; do
        echo ">> [$VAR] P = $P"
        LOG=$(OMP_NUM_THREADS=$P OMP_PROC_BIND=close OMP_PLACES=cores ./$BIN < "$ENTRADA" 2>&1 >/dev/null)
        echo "$LOG" | grep -E 'Benchmark'
        echo "$LOG" | grep -q 'Validacao P4 OK' || { echo "ERRO: validacao falhou ($VAR, P=$P)"; exit 1; }
        TPAR=$(echo "$LOG" | sed -n 's/.*T_par (mediana) = \([0-9.]*\).*/\1/p')
        TSEQ=$(echo "$LOG" | sed -n 's/.*T_seq = \([0-9.]*\).*/\1/p')
        echo "$P,$TPAR,$TSEQ" >> "$CSV"
    done
done
python3 ../../scripts/plot_speedup.py resultados_p4.csv "P4 - Histograma (Abordagem A: privado + aligned(64))" graficos_p4.png
python3 ../../scripts/plot_speedup.py resultados_p4_B.csv "P4 - Histograma (Abordagem B: reduction em array)" graficos_p4_B.png
echo; echo ">> Anti-exemplo atomic (compare com T_seq):"; cat resultados_p4_atomic.csv
