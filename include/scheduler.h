#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

/* Enumeração contendo os algoritmos de escalonamento de CPU suportados pelo simulador */
typedef enum {
    ESCALONADOR_ROUND_ROBIN,          /* Chaveamento por fatias de tempo (Quantum) */
    ESCALONADOR_SJF_PREEMPTIVO,       /* Shortest Job First com preempção (também conhecido como SRTF) */
    ESCALONADOR_PRIORIDADE_PREEMPTIVA /* Executa o de maior prioridade (menor número) e permite interrupções */
} TipoEscalonador;

/* 
 * Estrutura que representa um "recorte" de tempo em que a CPU esteve ocupada.
 * Utilizada para desenhar o gráfico de Gantt (Linha do Tempo) na interface.
 */
typedef struct {
    int id_processo;  /* O PID do processo executando. Se for -1, significa que a CPU estava ociosa (idle) */
    int tempo_inicio; /* Instante exato em que este trecho começou */
    int tempo_fim;    /* Instante exato em que este trecho terminou */
} EntradaLinhaDoTempo;

/* 
 * Estrutura para consolidar o resultado completo de uma simulação de escalonamento,
 * incluindo a linha do tempo desenhável e as estatísticas de desempenho gerais.
 */
typedef struct {
    EntradaLinhaDoTempo entradas[4096]; /* O histórico do que rodou na CPU passo a passo */
    int           quantidade_entradas;  /* O tamanho atual do histórico da linha do tempo */
    double        media_tempo_espera;   /* Média geral do Tempo de Espera (Waiting Time) de todos os processos */
    double        media_tempo_resposta; /* Média geral do Tempo de Resposta (Response Time) de todos os processos */
    double        media_tempo_retorno;  /* Média geral do Tempo de Retorno (Turnaround Time) de todos os processos */
} ResultadoEscalonador;

/* 
 * Função principal (Dispatcher) que executa a simulação do escalonador escolhido.
 * Parâmetros:
 * - processos[]       : vetor de processos base (os dados serão atualizados pela simulação).
 * - quantidade        : total de processos carregados.
 * - tipo_algoritmo    : qual algoritmo usar (Round Robin, SJF ou Prioridade).
 * - quantum           : o tempo máximo contínuo na CPU, obrigatório apenas para Round Robin.
 * - resultado         : a estrutura onde salvaremos as métricas e a timeline geradas.
 */
void executar_escalonador(Processo processos[], int quantidade,
                          TipoEscalonador tipo_algoritmo, int quantum,
                          ResultadoEscalonador *resultado);

/* --- Funções específicas de cada algoritmo (implementadas em scheduler.c) --- */
void escalonar_round_robin(Processo processos[], int quantidade, int quantum, ResultadoEscalonador *resultado);
void escalonar_sjf_preemptivo(Processo processos[], int quantidade, ResultadoEscalonador *resultado);
void escalonar_prioridade_preemptiva(Processo processos[], int quantidade, ResultadoEscalonador *resultado);

/* 
 * Função utilitária que varre a lista de processos finalizados e calcula as métricas finais (turnaround, espera, etc).
 */
void calcular_metricas(Processo processos[], int quantidade, ResultadoEscalonador *resultado);

#endif /* SCHEDULER_H */
