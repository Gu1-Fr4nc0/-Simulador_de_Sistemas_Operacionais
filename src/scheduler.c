#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/scheduler.h"

/* ------------------------------------------------------------------ */
/* Utilitários Internos do Escalonador                                */
/* ------------------------------------------------------------------ */

/* 
 * Função: adicionar_linha_do_tempo
 * Objetivo: Registrar um evento de uso da CPU na estrutura de resultados.
 */
static void adicionar_linha_do_tempo(ResultadoEscalonador *resultado, int id_processo, int tempo_inicio, int tempo_fim) {
    if (resultado->quantidade_entradas >= 4096) return; /* Evita estouro de buffer */
    
    resultado->entradas[resultado->quantidade_entradas].id_processo  = id_processo;
    resultado->entradas[resultado->quantidade_entradas].tempo_inicio = tempo_inicio;
    resultado->entradas[resultado->quantidade_entradas].tempo_fim    = tempo_fim;
    
    resultado->quantidade_entradas++;
}

/* 
 * Função: calcular_metricas
 * Objetivo: Calcular os tempos médios (Espera, Resposta e Retorno/Turnaround) baseando-se nos processos finalizados.
 */
void calcular_metricas(Processo processos[], int quantidade, ResultadoEscalonador *resultado) {
    double total_espera = 0, total_resposta = 0, total_retorno = 0;
    
    for (int i = 0; i < quantidade; i++) {
        Processo *p = &processos[i];
        
        /* Turnaround = Tempo de Fim - Tempo de Chegada */
        p->tempo_retorno = p->tempo_fim - p->tempo_chegada;
        
        /* Espera = Turnaround - Tempo Total Necessário de CPU */
        p->tempo_espera  = p->tempo_retorno - p->tempo_execucao;
        
        /* Resposta = Primeiro Uso da CPU - Tempo de Chegada */
        p->tempo_resposta = p->tempo_inicio - p->tempo_chegada;

        total_espera   += p->tempo_espera;
        total_resposta += p->tempo_resposta;
        total_retorno  += p->tempo_retorno;
    }
    
    /* Preenche o consolidado no ponteiro de resultado */
    resultado->media_tempo_espera   = total_espera / quantidade;
    resultado->media_tempo_resposta = total_resposta / quantidade;
    resultado->media_tempo_retorno  = total_retorno / quantidade;
}


/* ------------------------------------------------------------------ */
/* Algoritmo: Round-Robin (Chaveamento Circular com Quantum)          */
/* ------------------------------------------------------------------ */

void escalonar_round_robin(Processo processos[], int quantidade, int quantum, ResultadoEscalonador *resultado) {
    /* Cria uma cópia local do array de processos para que a simulação não destrua os dados originais no meio do caminho */
    Processo ps[MAX_PROCESSOS];
    memcpy(ps, processos, quantidade * sizeof(Processo));

    /* Fila circular simples implementada num array gigante */
    int fila[MAX_PROCESSOS * 100];
    int cabeca = 0, cauda = 0;
    int tempo_atual = 0;
    int concluidos = 0;

    /* Macros para enfileirar e desenfileirar usando aritmética modular (circular) */
    #define ENFILEIRAR(indice) fila[cauda++ % (MAX_PROCESSOS*100)] = (indice)
    #define DESENFILEIRAR()    fila[cabeca++ % (MAX_PROCESSOS*100)]

    /* Fase inicial: Adiciona na fila processos que já estão no sistema no tempo 0 */
    for (int i = 0; i < quantidade; i++)
        if (ps[i].tempo_chegada == 0) ENFILEIRAR(i);

    /* Laço principal da simulação até que todos acabem */
    while (concluidos < quantidade) {
        if (cabeca == cauda) { 
            /* Se a fila está vazia, a CPU fica ociosa (idle) por 1 tick de tempo */
            adicionar_linha_do_tempo(resultado, -1, tempo_atual, tempo_atual + 1);
            tempo_atual++;
            
            /* Verifica se algum processo chegou exatamente agora */
            for (int i = 0; i < quantidade; i++)
                if (ps[i].tempo_chegada == tempo_atual && ps[i].estado == ESTADO_NOVO)
                    ENFILEIRAR(i);
            continue;
        }

        /* Tira o próximo processo da fila */
        int indice = DESENFILEIRAR();
        Processo *p = &ps[indice];

        /* Se for a primeira vez que ele executa, marca o start_time para o Tempo de Resposta */
        if (p->estado == ESTADO_NOVO || p->estado == ESTADO_PRONTO) {
            if (p->tempo_inicio == -1) p->tempo_inicio = tempo_atual;
            p->estado = ESTADO_EXECUTANDO;
        }

        /* O processo roda o mínimo entre o que falta pra ele acabar e o limite de tempo (Quantum) */
        int execucao_agora = (p->tempo_restante < quantum) ? p->tempo_restante : quantum;
        
        /* Registra na timeline visual o evento de execução */
        adicionar_linha_do_tempo(resultado, p->id_processo, tempo_atual, tempo_atual + execucao_agora);
        
        p->tempo_restante -= execucao_agora;
        tempo_atual += execucao_agora;

        /* Importante: Verifica se enquanto este processo rodava, novos processos chegaram na fila */
        for (int i = 0; i < quantidade; i++) {
            if (ps[i].tempo_chegada > tempo_atual - execucao_agora &&
                ps[i].tempo_chegada <= tempo_atual &&
                ps[i].estado == ESTADO_NOVO) {
                ps[i].estado = ESTADO_PRONTO;
                ENFILEIRAR(i);
            }
        }

        /* Ao fim do quantum, verifica se o processo terminou... */
        if (p->tempo_restante == 0) {
            p->estado      = ESTADO_TERMINADO;
            p->tempo_fim   = tempo_atual;
            concluidos++;
        } else {
            /* ... se não, volta pra fila de prontos */
            p->estado = ESTADO_PRONTO;
            ENFILEIRAR(indice);
        }
    }

    /* Copia os dados resultantes modificados de volta para o array original */
    memcpy(processos, ps, quantidade * sizeof(Processo));
    
    /* Calcula o resultado estatístico */
    calcular_metricas(processos, quantidade, resultado);
    
    #undef ENFILEIRAR
    #undef DESENFILEIRAR
}

/* ------------------------------------------------------------------ */
/* Algoritmo: SJF Preemptivo (Shortest Remaining Time First - SRTF)   */
/* ------------------------------------------------------------------ */

void escalonar_sjf_preemptivo(Processo processos[], int quantidade, ResultadoEscalonador *resultado) {
    /* Cópia de trabalho */
    Processo ps[MAX_PROCESSOS];
    memcpy(ps, processos, quantidade * sizeof(Processo));

    int tempo_atual = 0, concluidos = 0;
    int pid_anterior = -1, inicio_fatia_tempo = 0;

    /* Executa simulando 1 unidade de tempo por vez */
    while (concluidos < quantidade) {
        
        /* Busca na lista de processos ativos o que tem o MENOR tempo restante (Shortest Job) */
        int melhor_indice = -1;
        for (int i = 0; i < quantidade; i++) {
            if (ps[i].tempo_chegada <= tempo_atual && ps[i].estado != ESTADO_TERMINADO) {
                if (melhor_indice == -1 || ps[i].tempo_restante < ps[melhor_indice].tempo_restante)
                    melhor_indice = i;
            }
        }

        /* Se ninguém está pronto, a CPU fica ociosa */
        if (melhor_indice == -1) {
            if (pid_anterior != -1) {
                adicionar_linha_do_tempo(resultado, pid_anterior, inicio_fatia_tempo, tempo_atual);
                pid_anterior = -1;
            }
            adicionar_linha_do_tempo(resultado, -1, tempo_atual, tempo_atual + 1);
            tempo_atual++;
            continue;
        }

        Processo *p = &ps[melhor_indice];
        if (p->tempo_inicio == -1) p->tempo_inicio = tempo_atual;
        if (p->estado != ESTADO_EXECUTANDO) p->estado = ESTADO_EXECUTANDO;

        /* Detecta se houve uma troca de contexto (preempção) para adicionar na timeline */
        if (p->id_processo != pid_anterior) {
            if (pid_anterior != -1)
                adicionar_linha_do_tempo(resultado, pid_anterior, inicio_fatia_tempo, tempo_atual);
            inicio_fatia_tempo = tempo_atual;
            pid_anterior = p->id_processo;
        }

        /* O processo roda por 1 unidade de tempo */
        p->tempo_restante--;
        tempo_atual++;

        /* Se terminou de executar, fecha os dados do processo */
        if (p->tempo_restante == 0) {
            p->estado      = ESTADO_TERMINADO;
            p->tempo_fim   = tempo_atual;
            concluidos++;
            
            adicionar_linha_do_tempo(resultado, p->id_processo, inicio_fatia_tempo, tempo_atual);
            pid_anterior = -1;
            inicio_fatia_tempo = tempo_atual;
        }
    }

    memcpy(processos, ps, quantidade * sizeof(Processo));
    calcular_metricas(processos, quantidade, resultado);
}

/* ------------------------------------------------------------------ */
/* Algoritmo: Prioridade Preemptiva                                   */
/* ------------------------------------------------------------------ */

void escalonar_prioridade_preemptiva(Processo processos[], int quantidade, ResultadoEscalonador *resultado) {
    /* Cópia de trabalho */
    Processo ps[MAX_PROCESSOS];
    memcpy(ps, processos, quantidade * sizeof(Processo));

    int tempo_atual = 0, concluidos = 0;
    int pid_anterior = -1, inicio_fatia_tempo = 0;

    while (concluidos < quantidade) {
        
        /* Busca na lista de processos ativos o que tem a MAIOR prioridade (Menor número) */
        int melhor_indice = -1;
        for (int i = 0; i < quantidade; i++) {
            if (ps[i].tempo_chegada <= tempo_atual && ps[i].estado != ESTADO_TERMINADO) {
                /* Prioridade numericamente menor significa que tem mais "poder" de interrupção */
                if (melhor_indice == -1 || ps[i].prioridade < ps[melhor_indice].prioridade)
                    melhor_indice = i;
            }
        }

        if (melhor_indice == -1) {
            if (pid_anterior != -1) {
                adicionar_linha_do_tempo(resultado, pid_anterior, inicio_fatia_tempo, tempo_atual);
                pid_anterior = -1;
            }
            adicionar_linha_do_tempo(resultado, -1, tempo_atual, tempo_atual + 1);
            tempo_atual++;
            continue;
        }

        Processo *p = &ps[melhor_indice];
        if (p->tempo_inicio == -1) p->tempo_inicio = tempo_atual;
        p->estado = ESTADO_EXECUTANDO;

        /* Preempção baseada em prioridade detectada */
        if (p->id_processo != pid_anterior) {
            if (pid_anterior != -1)
                adicionar_linha_do_tempo(resultado, pid_anterior, inicio_fatia_tempo, tempo_atual);
            inicio_fatia_tempo = tempo_atual;
            pid_anterior = p->id_processo;
        }

        p->tempo_restante--;
        tempo_atual++;

        if (p->tempo_restante == 0) {
            p->estado      = ESTADO_TERMINADO;
            p->tempo_fim   = tempo_atual;
            concluidos++;
            
            adicionar_linha_do_tempo(resultado, p->id_processo, inicio_fatia_tempo, tempo_atual);
            pid_anterior       = -1;
            inicio_fatia_tempo = tempo_atual;
        }
    }

    memcpy(processos, ps, quantidade * sizeof(Processo));
    calcular_metricas(processos, quantidade, resultado);
}

/* ------------------------------------------------------------------ */
/* Função Despachante (Dispatcher) Geral                              */
/* ------------------------------------------------------------------ */

void executar_escalonador(Processo processos[], int quantidade,
                          TipoEscalonador tipo_algoritmo, int quantum,
                          ResultadoEscalonador *resultado) {
                              
    /* Zera a estrutura de resultado */
    memset(resultado, 0, sizeof(ResultadoEscalonador));
    
    /* Chama a implementação correta de acordo com a seleção na interface */
    switch (tipo_algoritmo) {
        case ESCALONADOR_ROUND_ROBIN:
            escalonar_round_robin(processos, quantidade, quantum, resultado);
            break;
        case ESCALONADOR_SJF_PREEMPTIVO:
            escalonar_sjf_preemptivo(processos, quantidade, resultado);
            break;
        case ESCALONADOR_PRIORIDADE_PREEMPTIVA:
            escalonar_prioridade_preemptiva(processos, quantidade, resultado);
            break;
    }
}
