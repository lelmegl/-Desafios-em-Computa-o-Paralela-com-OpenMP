#!/usr/bin/env bash
# Benchmark oficial do P1: roda a versao base (P1_bench) e a variante simd (P1_bench_simd)
# para P em {1,2,4,6}. Salva resultados_p1.csv, resultados_p1_simd.csv, os graficos
# e log_p1.txt (com pi_seq e pi_par de cada rodada, para a Secao 2 do relatorio).
# Entrada: pi.in (N = 10^9, referencia do Apendice B.4). O ambiente vai para ambiente.txt.
# Uso (dentro de Iniciante/P1_pi):  ./bench_p1.sh
set -euo pipefail
cd "$(dirname "$0")"
source ../../scripts/ambiente.sh
make -s -B CC="$CC" CFLAGS="$CFLAGS" P1_bench P1_bench_simd
: > log_p1.txt
for VAR in base simd; do
    BIN=P1_bench; CSV=resultados_p1.csv
    [ "$VAR" = simd ] && { BIN=P1_bench_simd; CSV=resultados_p1_simd.csv; }
    echo "P,T_par_s,T_seq_s" > "$CSV"
    for P in 1 2 4 6; do
        echo ">> [$VAR] P = $P"
        LOG=$(OMP_NUM_THREADS=$P ./$BIN < ${ENTRADA_P1:-pi.in} 2>&1 >/dev/null)
        { echo "== $VAR P=$P"; echo "$LOG"; } >> log_p1.txt
        echo "$LOG" | grep -E 'pi_seq|Delta|Benchmark'
        echo "$LOG" | grep -q 'Benchmark P1' || { echo "ERRO: validacao falhou ($VAR, P=$P)"; exit 1; }
        TPAR=$(echo "$LOG" | sed -n 's/.*T_par (mediana) = \([0-9.]*\).*/\1/p')
        TSEQ=$(echo "$LOG" | sed -n 's/.*T_seq = \([0-9.]*\).*/\1/p')
        echo "$P,$TPAR,$TSEQ" >> "$CSV"
    done
done
python3 ../../scripts/plot_speedup.py resultados_p1.csv "P1 - Pi (parallel for reduction)" graficos_p1.png
python3 ../../scripts/plot_speedup.py resultados_p1_simd.csv "P1 - Pi (parallel for simd reduction)" graficos_p1_simd.png
