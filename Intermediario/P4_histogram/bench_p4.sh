#!/usr/bin/env bash
# Benchmark oficial do P4 (Membro 3): gera a entrada de estresse, roda P4_bench
# para P em {1,2,4,6}, valida byte a byte e salva os tempos em resultados_p4.csv.
# Uso (dentro de Intermediario/P4_histogram):  ./bench_p4.sh [M]   (padrao M = 50000000)
set -euo pipefail
cd "$(dirname "$0")"
M="${1:-50000000}"
ENTRADA="entrada_grande.in"   # fora do git (~175 MB para M = 5e7)

if [ ! -f "$ENTRADA" ] || [ "$(head -1 "$ENTRADA")" != "$M" ]; then
    echo ">> Gerando $ENTRADA com M = $M pixels..."
    python3 ../../gen_inputs.py p4 "$M" > "$ENTRADA"
fi

make -s P4_bench
echo "P,T_par_s,T_seq_s" > resultados_p4.csv
for P in 1 2 4 6; do
    echo ">> P = $P"
    LOG=$(OMP_NUM_THREADS=$P OMP_PROC_BIND=close OMP_PLACES=cores ./P4_bench < "$ENTRADA" 2>&1 >/dev/null)
    echo "$LOG" | grep -E 'Validacao|Benchmark'
    echo "$LOG" | grep -q 'Validacao P4 OK' || { echo "ERRO: validacao falhou em P=$P"; exit 1; }
    TPAR=$(echo "$LOG" | sed -n 's/.*T_par (mediana) = \([0-9.]*\).*/\1/p')
    TSEQ=$(echo "$LOG" | sed -n 's/.*T_seq = \([0-9.]*\).*/\1/p')
    echo "$P,$TPAR,$TSEQ" >> resultados_p4.csv
done
echo; echo ">> Tabela salva em resultados_p4.csv:"; cat resultados_p4.csv
python3 ../../scripts/plot_speedup.py resultados_p4.csv "P4 - Histograma de Cores" graficos_p4.png
