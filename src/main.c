/*
 * main.c — Ponto de entrada (Entry Point) do Simulador de SO
 *
 * Seu objetivo é delegar a execução para a interface gráfica em Qt 
 * contida no arquivo mainwindow.cpp.
 *
 * Nota: Caso o sistema precise rodar em terminal puro (CLI) para testes de 
 * depuração ou CI/CD, basta definir a macro CLI_MODE na compilação.
 */

#include <stdio.h>
#include "../include/mainwindow.h"

/* --- Modo Terminal/CLI (Apenas para Testes Rápidos sem Ambiente Gráfico) --- */
#ifdef CLI_MODE
#include "../include/process.h"
#include "../include/scheduler.h"
#include "../include/memory.h"
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso correto no modo CLI: %s <arquivo_processos.csv>\n", argv[0]);
        return 1;
    }

    /* Cria um array de processos com o limite máximo configurado */
    Processo processos[MAX_PROCESSOS];
    int quantidade_lida = carregar_processos_do_csv(argv[1], processos, MAX_PROCESSOS);
    
    if (quantidade_lida <= 0) return 1; /* Aborta se não carregou nada válido */

    printf("=== Processos Carregados do Arquivo ===\n");
    imprimir_processos(processos, quantidade_lida);

    /* Teste do Escalonador de Processos (Round-Robin fixo para testes CLI) */
    ResultadoEscalonador resultado_escalonador;
    memset(&resultado_escalonador, 0, sizeof(resultado_escalonador));
    executar_escalonador(processos, quantidade_lida, ESCALONADOR_ROUND_ROBIN, 4, &resultado_escalonador);

    printf("\n=== Gráfico de Gantt (Linha do Tempo - Round-Robin, Quantum=4) ===\n");
    for (int i = 0; i < resultado_escalonador.quantidade_entradas; i++) {
        EntradaLinhaDoTempo *entrada = &resultado_escalonador.entradas[i];
        
        if (entrada->id_processo == -1) {
            printf("[%2d-%2d] CPU Ociosa (Idle)\n", entrada->tempo_inicio, entrada->tempo_fim);
        } else {
            char *nome_processo = "?";
            for (int j = 0; j < quantidade_lida; j++) {
                if (processos[j].id_processo == entrada->id_processo) {
                    nome_processo = processos[j].nome;
                    break;
                }
            }
            printf("[%2d-%2d] Executando: P%d (%s)\n", entrada->tempo_inicio, entrada->tempo_fim, entrada->id_processo, nome_processo);
        }
    }

    /* Imprime as métricas do escalonador */
    printf("\n--- Estatísticas do Escalonador ---\n");
    printf("Tempo de Espera Médio   : %.2f\n", resultado_escalonador.media_tempo_espera);
    printf("Tempo de Resposta Médio : %.2f\n", resultado_escalonador.media_tempo_resposta);
    printf("Tempo de Retorno Médio  : %.2f\n", resultado_escalonador.media_tempo_retorno);

    /* Teste do Subsistema de Memória Virtual e Física */
    MemoriaFisica memoria = {0};
    ResultadoMemoria res_memoria = {0, 0};
    
    /* Simula 256MB de RAM física e 1024MB de espaço virtual em disco (Swap) */
    inicializar_memoria(&memoria, 256, 1024);

    TabelaPaginas tabela_paginas = {0};
    
    /* Carrega os processos um por um na memória para testar page faults com algoritmo LRU */
    for (int i = 0; i < quantidade_lida; i++) {
        memset(&tabela_paginas, 0, sizeof(tabela_paginas));
        
        int faults = carregar_processo_memoria(&memoria, &tabela_paginas, processos[i].id_processo,
                                               processos[i].memoria_necessaria,
                                               POLITICA_LRU,
                                               processos[i].tempo_chegada,
                                               processos, quantidade_lida, NULL);
                                               
        res_memoria.total_falhas_pagina += faults;
        res_memoria.total_acessos += ((processos[i].memoria_necessaria * 1024) / TAMANHO_PAGINA_KB) * 3;
        
        liberar_processo_memoria(&memoria, &tabela_paginas, processos[i].id_processo);
    }
    
    /* Imprime o relatório consolidado da memória */
    imprimir_estatisticas_memoria(&memoria, &res_memoria);
    
    return 0;
}
#else
/* --- Modo Interface Gráfica (Comportamento Padrão) ------------------- */
int main(int argc, char *argv[]) {
    /* Delega toda a execução visual para o Qt */
    return executar_interface_grafica(argc, argv);
}
#endif
