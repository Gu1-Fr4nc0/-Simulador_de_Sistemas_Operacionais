#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/process.h"

/* 
 * Formato esperado para as linhas do arquivo CSV de entrada:
 * id_processo,nome,tempo_chegada,tempo_execucao,prioridade,memoria_mb
 * 
 * Exemplo prático de linhas do CSV:
 *   1,P1,0,8,2,64
 *   2,P2,2,4,1,32
 * 
 * Nota: A primeira linha pode ser um cabeçalho explicativo, a função ignorará
 * se perceber que começa com letras ao invés de números.
 */
int carregar_processos_do_csv(const char *caminho_arquivo, Processo processos[], int capacidade_maxima) {
    /* Tenta abrir o arquivo CSV em modo leitura ("r") */
    FILE *arquivo = fopen(caminho_arquivo, "r");
    if (!arquivo) {
        /* Se houver falha (arquivo não existe, sem permissão, etc.), reporta no console */
        fprintf(stderr, "[ERRO] Não foi possível abrir o arquivo: %s\n", caminho_arquivo);
        return -1;
    }

    char linha_texto[256];
    int total_lidos = 0;

    /* Le a primeira linha do arquivo para verificar se é um cabeçalho */
    if (fgets(linha_texto, sizeof(linha_texto), arquivo)) {
        /* Se a linha começar com uma letra entre 'A' e 'z', consideramos que é cabeçalho e descartamos */
        if (linha_texto[0] >= 'A' && linha_texto[0] <= 'z') {
            /* Cabeçalho descartado, o loop while continuará lendo os próximos itens */
        } else {
            /* Se for um número, a primeira linha já era um dado válido. Retornamos o ponteiro ao início. */
            rewind(arquivo);
        }
    }

    /* Lê as demais linhas, limitando à capacidade máxima que o simulador suporta */
    while (fgets(linha_texto, sizeof(linha_texto), arquivo) && total_lidos < capacidade_maxima) {
        /* Pega uma referência ao processo na posição atual do vetor e zera a memória dele */
        Processo *p = &processos[total_lidos];
        memset(p, 0, sizeof(Processo));

        /* Extrai as variáveis da linha lida separada por vírgulas.
         * %31[^,] vai pegar uma string até no máximo 31 caracteres, parando se achar uma vírgula */
        int itens_extraidos = sscanf(linha_texto, "%d,%31[^,],%d,%d,%d,%d",
                                     &p->id_processo,
                                     p->nome,
                                     &p->tempo_chegada,
                                     &p->tempo_execucao,
                                     &p->prioridade,
                                     &p->memoria_necessaria);
        
        /* Mostra os dados lidos apenas para fins de monitoramento e depuração */
        printf("[DEBUG] PID=%d nome=%s chegada=%d execucao=%d prioridade=%d memoria_MB=%d\n",
               p->id_processo, p->nome, p->tempo_chegada, p->tempo_execucao,
               p->prioridade, p->memoria_necessaria);

        /* Valida se conseguiu capturar as 6 informações de forma correta */
        if (itens_extraidos < 6) {
            fprintf(stderr, "[AVISO] Linha ignorada (formato de dados inválido): %s", linha_texto);
            continue; /* Pula para a próxima linha */
        }

        /* Configurações padrões de inicialização de um processo novinho em folha */
        p->tempo_restante = p->tempo_execucao; /* Ainda precisa rodar todo seu tempo */
        p->estado         = ESTADO_NOVO;       /* Recém-chegado ao sistema */
        p->tempo_inicio   = -1;                /* Indica que nunca utilizou a CPU ainda */
        p->tempo_fim      = -1;                /* Ainda não finalizou, óbvio */
        
        /* Conta que foi efetuada uma leitura de processo válida */
        total_lidos++;
    }

    /* Fecha o descritor de arquivo aberto e devolve quantos processos leu do CSV */
    fclose(arquivo);
    return total_lidos;
}

/* 
 * Função utilitária que percorre o vetor e exibe os processos
 * na tela para que o usuário verifique os dados no modo CLI.
 */
void imprimir_processos(const Processo processos[], int quantidade) {
    /* Imprime o cabeçalho tabulado */
    printf("%-5s %-12s %-8s %-8s %-8s %-10s\n",
           "PID", "Nome", "Chegada", "Burst", "Prior.", "Mem(MB)");
    printf("----------------------------------------------------\n");
    
    /* Percorre todo o vetor */
    for (int i = 0; i < quantidade; i++) {
        const Processo *p = &processos[i];
        
        /* Imprime cada atributo respeitando o espaço reservado para manter o alinhamento */
        printf("%-5d %-12s %-8d %-8d %-8d %-10d\n",
               p->id_processo, p->nome, p->tempo_chegada,
               p->tempo_execucao, p->prioridade, p->memoria_necessaria);
    }
}
