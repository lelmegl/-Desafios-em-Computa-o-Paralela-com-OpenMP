# Desafios em Computação Paralela com OpenMP (Projeto 2)

Grupo: Marina Cantarelli Barroca (10740412), Guilherme Limeira de Souza (10439777) e Sofia Castelli (10443550).

Problemas: P1 – Cálculo de π (`Iniciante/P1_pi`), P4 – Histograma de Cores (`Intermediario/P4_histogram`) e P8 – Fractal de Mandelbrot (`Avancado/P8_mandelbrot`). Cada pasta tem `sequencial.c`, `paralelo.c`, `Makefile` e `entrada.in`.

## Validação

```bash
bash test.sh
```

Compila os três problemas e compara a saída paralela com a sequencial (P1 com tolerância relativa de 10⁻⁹; P4 e P8 byte a byte).

## Medições do relatório

Dentro da pasta de cada problema:

```bash
./bench_p1.sh   # P1: versão base e simd, N = 10^9
./bench_p4.sh   # P4: abordagens A, B e atomic; gera a entrada com M = 10^9 (~3,6 GB) usando gen_inputs.py
./bench_p8.sh   # P8: políticas de escalonamento e tabela oficial, 10.000 x 10.000, Imax = 10.000
```

Cada script roda P ∈ {1, 2, 4, 6} (1 aquecimento + mediana de 5), valida o resultado, salva os tempos em `.csv`, gera os gráficos e registra a máquina em `ambiente.txt`. O `entrada.in` do P4 é pequeno (validação); a entrada do benchmark é gerada pelo script porque não cabe no repositório.

Requisitos: `gcc` com OpenMP, `make`, `python3` e `matplotlib`.
