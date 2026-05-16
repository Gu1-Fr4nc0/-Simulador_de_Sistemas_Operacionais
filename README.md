# Simulador de Sistemas Operacionais
**Disciplina:** Sistemas Operacionais — UTFPR  
**Entrega:** 21/06 | **Apresentação:** 22 e 24/06

---

## Estrutura do Projeto

```
so_simulator/
├── include/
│   ├── process.h         ← estrutura Process, estados, prototipagem
│   ├── scheduler.h       ← tipos de escalonador, TimelineEntry, resultado
│   ├── memory.h          ← paginação, políticas de substituição
│   └── mainwindow.h      ← interface C para a GUI Qt
├── src/
│   ├── process.c         ← carregamento de CSV
│   ├── scheduler.c       ← RR, SJF-P, Prioridade-P
│   ├── memory.c          ← FIFO, LRU, Ótimo
│   ├── mainwindow.cpp    ← janela Qt (C++)
│   └── main.c            ← ponto de entrada
├── resources/
│   └── processos_exemplo.csv
├── Makefile
├── so_simulator.pro      ← projeto Qt Creator
└── README.md
```

---

## Compilação

### Opção 1 — Qt Creator (recomendado)
1. Abra `so_simulator.pro` no Qt Creator.
2. Configure o kit (Qt 5 ou Qt 6 + GCC).
3. Clique em **Build**.

### Opção 2 — Linha de Comando (Makefile)
```bash
# GUI (requer Qt5 ou Qt6 + pkg-config)
make

# CLI para testes rápidos sem GUI
make cli
```

### Dependências (Ubuntu/Debian)
```bash
sudo apt install qt6-base-dev qt6-tools-dev-tools pkg-config
# ou Qt5:
sudo apt install qtbase5-dev qttools5-dev-tools pkg-config
```

---

## Formato do CSV de Entrada

```
pid,nome,arrival_time,burst_time,priority,memory_mb
1,P1,0,8,3,64
2,P2,1,4,1,32
```

| Campo          | Descrição                                   |
|----------------|---------------------------------------------|
| `pid`          | Identificador do processo                   |
| `nome`         | Nome do processo (até 31 chars)             |
| `arrival_time` | Tempo de chegada                            |
| `burst_time`   | Tempo de execução (CPU Burst)               |
| `priority`     | Prioridade (menor número = maior prioridade)|
| `memory_mb`    | Memória necessária em MB                    |

---

## Algoritmos Implementados

### Escalonamento
| Algoritmo              | Descrição                                       |
|------------------------|-------------------------------------------------|
| Round-Robin (RR)       | Quantum configurável via GUI                    |
| SJF Preemptivo (SRTF)  | Troca processo se chegar um com menor burst     |
| Prioridade Preemptiva  | Menor número de prioridade = maior prioridade   |

### Substituição de Páginas
| Política | Descrição                                              |
|----------|--------------------------------------------------------|
| FIFO     | Remove a página mais antiga na memória                 |
| LRU      | Remove a menos recentemente usada                      |
| Ótimo    | Remove a que será usada mais tarde no futuro (offline) |

---

## Distribuição de Pontos (conforme especificação)

| Item                                        | Pontuação |
|---------------------------------------------|-----------|
| Interface Gráfica                           | 2.0       |
| Algoritmos de Escalonamento (RR, SJF-P, PP) | 4.0       |
| Memória Virtual e Substituição de Páginas   | 4.0       |
| **Total**                                   | **10.0**  |

---

## Onde Implementar o Restante

Os arquivos já têm os esqueletos completos. O que falta é:

- `scheduler.c` — os algoritmos já estão implementados; revise e teste.
- `memory.c` — FIFO, LRU e Ótimo já implementados; teste com o CSV.
- `mainwindow.cpp` — GUI funcional; adicione melhorias visuais se quiser.
- **Testes** — use `make cli` + o CSV de exemplo para validar a lógica antes de integrar à GUI.
