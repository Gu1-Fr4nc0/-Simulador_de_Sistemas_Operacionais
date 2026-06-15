#ifndef MAINWINDOW_H
#define MAINWINDOW_H

/*
 * JanelaPrincipal - Interface gráfica principal do Simulador de SO
 *
 * Estrutura da Interface (Qt Widgets):
 *
 *  ┌─────────────────────────────────────────────────────┐
 *  │  [Carregar CSV]   arquivo: ---                       │
 *  │  Algoritmo: [Round-Robin ▼]   Quantum: [4  ]         │
 *  │  Política Mem: [LRU ▼]  Mem Física: [256 MB]         │
 *  │                        Mem Virtual: [1024 MB]        │
 *  │  [▶ Simular]                                         │
 *  ├─────────────────────────────────────────────────────┤
 *  │  Linha do Tempo  (área de desenho/Gantt)             │
 *  ├─────────────────────────────────────────────────────┤
 *  │  Métricas:  Espera Média | Resposta Média | Faults   │
 *  │  Tabela de processos (QTableWidget)                  │
 *  └─────────────────────────────────────────────────────┘
 */

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * Função de entrada da Interface Gráfica. 
 * Responsável por instanciar a aplicação Qt e abrir a janela na tela.
 */
int executar_interface_grafica(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* MAINWINDOW_H */
