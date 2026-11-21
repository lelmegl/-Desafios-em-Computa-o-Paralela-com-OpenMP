#!/usr/bin/env python3
"""Gera a tabela P | T(P) | S(P) | E(P) e os 2 graficos exigidos (Secoes 3 e 4 do relatorio).

Uso: python3 scripts/plot_speedup.py resultados.csv "Titulo" saida.png
CSV: colunas P,T_par_s (outras colunas sao ignoradas). T(1) = T_par com P = 1.
"""
import csv, sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

arq, titulo, saida = sys.argv[1], sys.argv[2], sys.argv[3]
with open(arq) as f:
    linhas = sorted(csv.DictReader(f), key=lambda r: int(r["P"]))
P = [int(r["P"]) for r in linhas]
T = [float(r["T_par_s"]) for r in linhas]
T1 = T[P.index(1)]
S = [T1 / t for t in T]
E = [s / p for s, p in zip(S, P)]

print(f"\n{'P':>2} | {'T(P) (s)':>10} | {'S(P)':>6} | {'E(P)':>6}")
for p, t, s, e in zip(P, T, S, E):
    print(f"{p:>2} | {t:>10.4f} | {s:>6.2f} | {e:>6.2f}")

fig, (a1, a2) = plt.subplots(1, 2, figsize=(11, 4.2))
a1.plot(P, P, "--", color="gray", label="Ideal (y = P)")
a1.plot(P, S, "o-", color="black", label="Medido")
a1.set(xlabel="P (threads)", ylabel="S(P) = T(1)/T(P)", title="Speedup", xticks=P)
a2.plot(P, [1.0] * len(P), "--", color="gray", label="Ideal (y = 1)")
a2.plot(P, E, "o-", color="black", label="Medido")
a2.set(xlabel="P (threads)", ylabel="E(P) = S(P)/P", title="Eficiencia", xticks=P, ylim=(0, 1.15))
for a in (a1, a2):
    a.grid(alpha=0.3); a.legend()
fig.suptitle(titulo)
fig.tight_layout()
fig.savefig(saida, dpi=150)
print(f"\nGraficos salvos em {saida}")
