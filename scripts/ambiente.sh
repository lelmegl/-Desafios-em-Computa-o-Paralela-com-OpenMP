#!/usr/bin/env bash
# Detecta compilador/flags com OpenMP (Linux ou macOS) e registra o ambiente de medicao.
# Uso dentro dos scripts:  source ../../scripts/ambiente.sh
# Exporta: CC, CFLAGS (passados ao make) e gera ambiente.txt na pasta atual.
SO="$(uname -s)"
if [ -z "${CC:-}" ]; then
    if [ "$SO" = "Darwin" ]; then
        # O "gcc" do macOS e o clang da Apple (sem -fopenmp). Usa o GCC do Homebrew.
        for v in 16 15 14 13 12; do command -v "gcc-$v" >/dev/null 2>&1 && { CC="gcc-$v"; break; }; done
        [ -z "${CC:-}" ] && { echo "ERRO: instale o GCC do Homebrew:  brew install gcc" >&2; exit 1; }
    else
        CC=gcc
    fi
fi
ARCHFLAG=""
for f in "-march=native" "-mcpu=native"; do
    if echo 'int main(void){return 0;}' | "$CC" $f -fopenmp -x c - -o /tmp/_probe_cc 2>/dev/null; then ARCHFLAG="$f"; break; fi
done
CFLAGS="-O3 $ARCHFLAG -fopenmp -Wall -Wextra"
export CC CFLAGS
export OMP_PROC_BIND=close OMP_PLACES=cores   # no macOS o libgomp ignora a afinidade

[ "${SEM_AMBIENTE_TXT:-0}" = 1 ] && return 0 2>/dev/null
{
    echo "Data: $(date)"
    echo "SO: $(uname -a)"
    echo "Compilador: $("$CC" --version | head -1)"
    echo "Flags: $CFLAGS"
    if [ "$SO" = "Darwin" ]; then
        echo "CPU: $(sysctl -n machdep.cpu.brand_string)"
        echo "Nucleos fisicos: $(sysctl -n hw.physicalcpu) | logicos: $(sysctl -n hw.logicalcpu)"
        P0=$(sysctl -n hw.perflevel0.physicalcpu 2>/dev/null || true)
        P1=$(sysctl -n hw.perflevel1.physicalcpu 2>/dev/null || true)
        if [ -n "$P0" ]; then echo "Nucleos de desempenho (P): $P0 | eficiencia (E): ${P1:-0}"; fi
        echo "RAM: $(( $(sysctl -n hw.memsize) / 1073741824 )) GB"
        echo "Linha de cache: $(sysctl -n hw.cachelinesize) bytes"
    else
        lscpu | grep -E "Model name|Thread\(s\)|Core\(s\)|Socket|L1d|L2|L3"
        echo "nproc: $(nproc)"
        if grep -qi microsoft /proc/version; then echo "Ambiente: WSL2"; else echo "Ambiente: Linux nativo"; fi
    fi
} > ambiente.txt
true
