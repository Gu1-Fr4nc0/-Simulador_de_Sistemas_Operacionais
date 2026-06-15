#ifndef MEMORY_H
#define MEMORY_H

#include "process.h"

/* Constantes e configurações do subsistema de Memória */
#define TAMANHO_PAGINA_KB 4      /* Define que cada página/frame terá 4 KB */
#define MAX_QUADROS_FISICOS 1024 /* Quantidade máxima de frames físicos permitidos */
#define MAX_PAGINAS_VIRTUAIS 2048 /* Quantidade máxima de páginas virtuais */

/* Algoritmos de Substituição de Página suportados pelo simulador */
typedef enum {
    POLITICA_FIFO,     /* First In, First Out (Primeiro a Entrar, Primeiro a Sair) */
    POLITICA_LRU,      /* Least Recently Used (Menos Recentemente Usado) */
    POLITICA_OTIMA     /* Algoritmo Ótimo (Substitui a página que demorará mais para ser usada) */
} PoliticaSubstituicaoPagina;

/* Representa uma única entrada (linha) na Tabela de Páginas do Processo */
typedef struct {
    int numero_pagina; /* Número lógico da página do processo */
    int numero_quadro; /* Número físico do frame na memória principal (se estiver lá) */
    int valido;        /* Bit de validade: 1 = na memória física, 0 = apenas na memória virtual (disco) */
    int ultimo_uso;    /* Marca de tempo do último acesso (usado no algoritmo LRU) */
    int tempo_carga;   /* Marca de tempo de quando foi carregada na memória (usado no algoritmo FIFO) */
} EntradaTabelaPagina;

/* Tabela de Páginas exclusiva de um processo específico */
typedef struct {
    int id_processo;                 /* A qual PID esta tabela pertence */
    EntradaTabelaPagina *paginas;    /* Ponteiro para o array dinâmico de páginas */
    int quantidade_paginas;          /* Quantas páginas estão efetivamente alocadas na memória */
    int max_paginas;                 /* O máximo de páginas que este processo precisa */
    int dica_vitima;                 /* Ponteiro de controle para algoritmo FIFO circular (ponteiro de varredura) */
} TabelaPaginas;

/* Estrutura que gerencia a Memória RAM (física) do simulador */
typedef struct {
    int *quadros;             /* Array que guarda o PID do processo que ocupa cada quadro (-1 se estiver livre) */
    int total_quadros;        /* Total de quadros (frames) disponíveis na RAM */
    int quadros_em_uso;       /* Quantos quadros estão ocupados atualmente */
} MemoriaFisica;

/* Agrupa os resultados das simulações de acesso à memória */
typedef struct {
    int total_falhas_pagina; /* Contador total de Page Faults */
    int total_acessos;       /* Contador total de acessos de memória solicitados */
} ResultadoMemoria;

/* 
 * Inicializa a estrutura da memória física e prepara o ambiente virtual.
 * Parâmetros recebidos vêm da interface ou CLI. 
 */
void inicializar_memoria(MemoriaFisica *memoria, int fisica_mb, int virtual_mb);

/* 
 * Tenta carregar todas as páginas de um processo conforme necessidade.
 * Retorna o número de page faults gerados durante o carregamento inicial.
 */
int carregar_processo_memoria(MemoriaFisica *memoria, TabelaPaginas *tabela,
                              int id_processo, int memoria_necessaria_mb,
                              PoliticaSubstituicaoPagina politica, int tempo_atual,
                              Processo *todos_processos, int num_processos, int *referencias_futuras);

/* 
 * Quando um processo termina, este método libera todos os quadros (frames) 
 * que ele ocupava, tornando-os disponíveis novamente.
 */
void liberar_processo_memoria(MemoriaFisica *memoria, TabelaPaginas *tabela, int id_processo);

/* 
 * Simula a CPU tentando acessar uma página.
 * Retorna 1 se ocorrer Page Fault (precisou buscar do disco) ou 0 se der Hit (já estava na RAM).
 */
int acessar_pagina_memoria(MemoriaFisica *memoria, TabelaPaginas *tabela,
                           int numero_pagina, PoliticaSubstituicaoPagina politica,
                           int tempo_atual, int *referencias_futuras, int tamanho_referencias);

/* 
 * Imprime um resumo do uso da memória e a taxa de page faults gerados.
 */
void imprimir_estatisticas_memoria(const MemoriaFisica *memoria, const ResultadoMemoria *resultado);

#endif /* MEMORY_H */
