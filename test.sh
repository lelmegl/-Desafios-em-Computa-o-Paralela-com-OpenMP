#!/usr/bin/env bash
# Script de validação adaptado do Apêndice B.6 do Caderno para os 3 problemas do grupo (P1, P4 e P8)
set -uo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
FAIL=0
# Compilador com OpenMP (no macOS: GCC do Homebrew). Ver scripts/ambiente.sh
SEM_AMBIENTE_TXT=1 source "$ROOT/scripts/ambiente.sh" || exit 1

run_c() { # $1=dir $2=binary $3=stdin $4=expected $5=is_float
    local dir="$1" bin="$2" in="$3" exp="$4" is_float="${5:-0}"
    echo "== $dir =="
    ( cd "$ROOT/$dir" && make -s CC="$CC" CFLAGS="$CFLAGS" ) || { echo "  COMPILE FAIL"; FAIL=1; return; }
    if [ ! -f "$ROOT/$dir/$exp" ]; then
        ( cd "$ROOT/$dir" && ./sequencial < "$in" > "$exp" 2>/dev/null )
        echo "  (gerado $exp a partir do sequencial de referencia)"
    fi
    if [ "$is_float" -eq 1 ]; then
        # Para P1 (ponto flutuante), verifica tolerancia relativa <= 1e-9 via awk
        local out_val exp_val
        out_val=$(cd "$ROOT/$dir" && ./"$bin" < "$in" 2>/dev/null)
        exp_val=$(cat "$ROOT/$dir/$exp")
        if awk -v a="$exp_val" -v b="$out_val" 'BEGIN {
            diff = (a > b) ? (a - b) : (b - a);
            rel = (a != 0) ? (diff / ((a > 0) ? a : -a)) : diff;
            exit (rel <= 1e-9) ? 0 : 1;
        }'; then
            echo "  OK (|Delta_rel| <= 10^-9)"
        else
            echo "  DIFF (Fora da tolerancia: esperado=$exp_val vs obtido=$out_val)"; FAIL=1
        fi
    else
        if diff -q <(cd "$ROOT/$dir" && ./"$bin" < "$in" 2>/dev/null) "$ROOT/$dir/$exp" >/dev/null; then
            echo "  OK (Igualdade exata byte a byte)"
        else
            echo "  DIFF"; FAIL=1
        fi
    fi
}

run_c Iniciante/P1_pi            P1_pi          pi.in          pi.expected          1
run_c Intermediario/P4_histogram P4_histogram   histogram.in   histogram.expected   0
run_c Avancado/P8_mandelbrot     P8_mandelbrot  mandelbrot.in  mandelbrot.expected  0

exit $FAIL
