#!/usr/bin/env python3
"""
Gerador de entradas de estresse/referência (Apêndice B.4 do Caderno)
para medir Speedup S(P) e Eficiência E(P) com P em {1, 2, 4, 6}.

Uso:
  python3 gen_inputs.py p1 > Iniciante/P1_pi/entrada.in
  python3 gen_inputs.py p4 [M] > Intermediario/P4_histogram/entrada.in
  python3 gen_inputs.py p8 [W] [H] [Imax] > Avancado/P8_mandelbrot/entrada.in
"""
import sys
import random

def main():
    if len(sys.argv) < 2:
        print(__doc__, file=sys.stderr)
        sys.exit(1)

    prob = sys.argv[1].lower()
    random.seed(42)

    if prob == "p1":
        # Referencia oficial do caderno (B.4): N = 10^9
        N = int(sys.argv[2]) if len(sys.argv) > 2 else 1000000000
        print(N)
    elif prob == "p4":
        # Referencia do caderno (B.4): M até 10^9 (use 10^7 ou 10^8 para teste rápido em texto)
        M = int(sys.argv[2]) if len(sys.argv) > 2 else 10000000
        print(M)
        chunk = [str(random.randint(0, 255)) for _ in range(min(M, 10000))]
        written = 0
        while written < M:
            take = min(len(chunk), M - written)
            sys.stdout.write(" ".join(chunk[:take]) + "\n")
            written += take
    elif prob == "p8":
        # Referencia oficial do caderno (B.4): W = H = 10^4, Imax = 10^4
        # (Para testes rapidos pode passar ex: 2000 2000 5000)
        W = int(sys.argv[2]) if len(sys.argv) > 2 else 10000
        H = int(sys.argv[3]) if len(sys.argv) > 3 else 10000
        Imax = int(sys.argv[4]) if len(sys.argv) > 4 else 10000
        print(f"{W} {H} {Imax}")
    else:
        print(f"Problema desconhecido: {prob}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
