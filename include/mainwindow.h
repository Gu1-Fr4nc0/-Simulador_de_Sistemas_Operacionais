#ifndef MAINWINDOW_H
#define MAINWINDOW_H

/*
 * MainWindow - Janela principal do Simulador de SO
 *
 * Estrutura da GUI (Qt Widgets):
 *
 *  ┌─────────────────────────────────────────────────────┐
 *  │  [Carregar CSV]   arquivo: ---                       │
 *  │  Algoritmo: [Round-Robin ▼]   Quantum: [4  ]         │
 *  │  Política Mem: [LRU ▼]  Mem Física: [256 MB]         │
 *  │                        Mem Virtual: [1024 MB]        │
 *  │  [▶ Simular]                                         │
 *  ├─────────────────────────────────────────────────────┤
 *  │  Linha do Tempo  (canvas de desenho)                 │
 *  ├─────────────────────────────────────────────────────┤
 *  │  Métricas:  Espera Média | Resposta Média | Faults   │
 *  │  Tabela de processos (QTableWidget)                  │
 *  └─────────────────────────────────────────────────────┘
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Ponto de entrada da GUI; chama QApplication e exibe a janela */
int gui_run(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* MAINWINDOW_H */
