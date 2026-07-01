#ifndef PROCESS_H
#define PROCESS_H

/* Constantes para definir os limites do sistema */
#define MAX_PROCESSOS 256
#define MAX_TAMANHO_NOME 32

/* Enumeração que define os possíveis estados de um processo durante sua vida no SO */
typedef enum {
    ESTADO_NOVO,        /* Processo acabou de chegar e foi criado */
    ESTADO_PRONTO,      /* Processo está na fila, pronto para usar a CPU */
    ESTADO_EXECUTANDO,  /* Processo está atualmente em execução na CPU */
    ESTADO_ESPERANDO,   /* Processo aguarda algum evento (não utilizado ativamente nesta versão básica) */
    ESTADO_TERMINADO    /* Processo concluiu toda a sua execução */
} EstadoProcesso;

#define MAX_HISTORICO_MEMORIA 512

/* Representa uma entrada individual no histórico de acessos à memória */
typedef struct {
    int pagina;
    int falha; /* 1 = Page Fault, 0 = Page Hit */
} EntradaHistoricoMemoria;

/* Estrutura que representa um Processo no nosso Simulador */
typedef struct {
    int   id_processo;                 /* Identificador único do processo (PID) */
    char  nome[MAX_TAMANHO_NOME];      /* Nome ou apelido do processo (ex: P1, Proc_A) */
    
    /* Atributos carregados do arquivo de entrada (CSV) */
    int   tempo_chegada;               /* Instante de tempo em que o processo entra no simulador */
    int   tempo_execucao;              /* Quantidade total de tempo de CPU que o processo precisa (Burst Time) */
    int   tempo_restante;              /* Tempo de CPU que ainda falta para o processo terminar (usado na preempção) */
    int   prioridade;                  /* Prioridade de execução (valores menores significam maior prioridade) */
    int   memoria_necessaria;          /* Tamanho de memória física que o processo requer em MB */
    
    /* Controle de estado do processo */
    EstadoProcesso estado;             /* Qual o estado atual deste processo */

    /* Variáveis para cálculo de métricas al final da simulação */
    int   tempo_inicio;                /* Instante de tempo em que o processo rodou pela primeira vez na CPU */
    int   tempo_fim;                   /* Instante de tempo em que o processo finalizou sua execução */
    int   tempo_espera;                /* Tempo total em que o processo ficou esperando na fila de prontos */
    int   tempo_resposta;              /* Tempo gasto desde a chegada até o primeiro acesso à CPU */
    int   tempo_retorno;               /* Tempo total do processo no sistema (desde a chegada até o fim - Turnaround) */

    /* Histórico de requisições de página para visualização */
    EntradaHistoricoMemoria historico_acessos[MAX_HISTORICO_MEMORIA];
    int quantidade_acessos;
} Processo;

/* 
 * Função: carregar_processos_do_csv
 * Objetivo: Ler um arquivo de texto no formato CSV e popular o vetor de processos.
 * Parâmetros:
 *   - caminho_arquivo: Onde o CSV está no disco.
 *   - processos: O vetor que armazenará os dados lidos.
 *   - capacidade_maxima: O tamanho máximo que o vetor suporta.
 * Retorno: A quantidade de processos lidos com sucesso ou -1 se houver falha.
 */
int carregar_processos_do_csv(const char *caminho_arquivo, Processo processos[], int capacidade_maxima);

/* 
 * Função: imprimir_processos
 * Objetivo: Mostrar a lista de processos no terminal (usado para depuração/debug).
 */
void imprimir_processos(const Processo processos[], int quantidade);

#endif /* PROCESS_H */

