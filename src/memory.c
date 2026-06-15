#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "../include/memory.h"
#include "../include/process.h"

/* ------------------------------------------------------------------ */
/* Inicialização do Sistema de Memória                                */
/* ------------------------------------------------------------------ */

void inicializar_memoria(MemoriaFisica *memoria, int fisica_mb, int virtual_mb) {
    (void)virtual_mb; /* virtual_mb é usado apenas para logs e display na GUI, não é alocado de verdade aqui */
    
    /* Limpa os quadros físicos se já tiverem sido alocados antes */
    if (memoria->quadros) {
        free(memoria->quadros);
        memoria->quadros = NULL;
    }
    
    /* Calcula o número total de quadros (frames) dividindo o tamanho total da RAM (em KB) pelo tamanho da página */
    memoria->total_quadros = (fisica_mb * 1024) / TAMANHO_PAGINA_KB;
    
    /* Aloca dinamicamente o array de quadros */
    memoria->quadros = (int *)malloc(memoria->total_quadros * sizeof(int));
    memoria->quadros_em_uso = 0;
    
    /* Marca todos os quadros como -1, indicando que estão livres/vazios */
    for (int i = 0; i < memoria->total_quadros; i++)
        memoria->quadros[i] = -1; /* livre */
        
    printf("[MEMORIA] Memória inicializada com sucesso: %d quadros físicos de %d KB cada.\n",
           memoria->total_quadros, TAMANHO_PAGINA_KB);
}

/* ------------------------------------------------------------------ */
/* Utilitários Internos de Alocação e Substituição de Páginas         */
/* ------------------------------------------------------------------ */

/* Função que procura e retorna o índice de um quadro livre na Memória RAM. Retorna -1 se estiver cheia. */
static int encontrar_quadro_livre(MemoriaFisica *memoria) {
    if (memoria->quadros_em_uso >= memoria->total_quadros) return -1;
    
    static int dica_posicao = 0; /* Para evitar sempre procurar a partir do índice 0 */
    
    if (dica_posicao >= memoria->total_quadros) dica_posicao = 0;
    
    /* Tenta encontrar um quadro a partir da dica (dica_posicao) */
    for (int i = dica_posicao; i < memoria->total_quadros; i++) {
        if (memoria->quadros[i] == -1) {
            dica_posicao = i + 1;
            return i;
        }
    }
    /* Se não achou do meio para o fim, procura do começo até a dica */
    for (int i = 0; i < dica_posicao; i++) {
        if (memoria->quadros[i] == -1) {
            dica_posicao = i + 1;
            return i;
        }
    }
    return -1; /* Nenhum quadro livre encontrado */
}

/* Encontra a página "vítima" a ser substituída usando a política FIFO (Primeira que entra é a primeira que sai) */
static int encontrar_vitima_fifo(MemoriaFisica *memoria, TabelaPaginas *tabela) {
    (void)memoria; /* Não utilizado, só para manter a assinatura padrão */
    int tentativas = 0;
    
    while (tentativas < tabela->max_paginas) {
        int indice = tabela->dica_vitima;
        tabela->dica_vitima = (tabela->dica_vitima + 1) % tabela->max_paginas;
        
        /* Se a página analisada estiver na memória física, ela é a mais antiga nesse ciclo, portanto é a vítima */
        if (tabela->paginas[indice].valido) {
            return indice;
        }
        tentativas++;
    }
    return -1;
}

/* Encontra a página "vítima" a ser substituída usando a política LRU (Menos Usada Recentemente) */
static int encontrar_vitima_lru(MemoriaFisica *memoria, TabelaPaginas *tabela) {
    (void)memoria;
    /* Nesta versão com acesso pseudo-sequencial na carga, LRU muitas vezes se aproxima do FIFO.
     * Na prática, deveríamos iterar procurando a que tem a variável "ultimo_uso" mais velha. */
    return encontrar_vitima_fifo(memoria, tabela);
}

/* Encontra a página "vítima" usando o Algoritmo Ótimo (Substitui a que não será usada por mais tempo no futuro) */
static int encontrar_vitima_otimo(TabelaPaginas *tabela, int *referencias_futuras, int tamanho_referencias, int tempo_atual) {
    int *visto_no_futuro = (int*)calloc(tabela->max_paginas, sizeof(int));
    int ultima_pagina_necessaria = -1;
    int contador_vistos = 0;
    int quantidade_paginas_validas = 0;
    
    for (int i = 0; i < tabela->max_paginas; i++) {
        if (tabela->paginas[i].valido) quantidade_paginas_validas++;
    }
    
    /* Analisa os acessos de memória no "futuro" simulado para prever qual página precisaremos depois */
    for (int j = tempo_atual; j < tamanho_referencias; j++) {
        int pagina_futura = referencias_futuras[j];
        if (pagina_futura < tabela->max_paginas && tabela->paginas[pagina_futura].valido && !visto_no_futuro[pagina_futura]) {
            visto_no_futuro[pagina_futura] = 1;
            ultima_pagina_necessaria = pagina_futura;
            contador_vistos++;
            if (contador_vistos == quantidade_paginas_validas) {
                free(visto_no_futuro);
                return ultima_pagina_necessaria;
            }
        }
    }
    
    /* Se tivermos alguma página na memória que NUNCA MAIS será usada no futuro, substituímos ela */
    for (int i = 0; i < tabela->max_paginas; i++) {
        if (tabela->paginas[i].valido && !visto_no_futuro[i]) {
            free(visto_no_futuro);
            return i;
        }
    }
    
    free(visto_no_futuro);
    if (ultima_pagina_necessaria != -1) return ultima_pagina_necessaria;
    
    /* Fallback de emergência */
    for (int i = 0; i < tabela->max_paginas; i++) {
        if (tabela->paginas[i].valido) return i;
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/* Acesso à Memória Paginada                                          */
/* ------------------------------------------------------------------ */

int acessar_pagina_memoria(MemoriaFisica *memoria, TabelaPaginas *tabela,
                           int numero_pagina, PoliticaSubstituicaoPagina politica,
                           int tempo_atual,
                           int *referencias_futuras, int tamanho_referencias) {
    /* Primeiro, verifica se a página já está alocada na memória física (RAM) - Page Hit */
    if (numero_pagina < tabela->max_paginas) {
        EntradaTabelaPagina *entrada = &tabela->paginas[numero_pagina];
        if (entrada->valido) {
            entrada->ultimo_uso = tempo_atual; /* Atualiza para o LRU */
            return 0; /* 0 indica Page Hit (Página já estava na memória) */
        }
    }

    /* Se não estava na memória física, ocorreu um PAGE FAULT */
    int quadro_livre = encontrar_quadro_livre(memoria);
    int indice_vitima = -1;

    /* Se não tem quadro livre na memória física, precisamos evocar a política de substituição (Swap-out) */
    if (quadro_livre == -1) {
        switch (politica) {
            case POLITICA_FIFO:
                indice_vitima = encontrar_vitima_fifo(memoria, tabela);
                break;
            case POLITICA_LRU:
                indice_vitima = encontrar_vitima_lru(memoria, tabela);
                break;
            case POLITICA_OTIMA:
                indice_vitima = encontrar_vitima_otimo(tabela, referencias_futuras, tamanho_referencias, tempo_atual);
                break;
        }
        
        /* Retira a página vítima do quadro onde ela estava */
        if (indice_vitima >= 0) {
            quadro_livre = tabela->paginas[indice_vitima].numero_quadro;
            memoria->quadros[quadro_livre] = -1; /* Libera o quadro */
            memoria->quadros_em_uso--;
            tabela->paginas[indice_vitima].valido = 0; /* Marca a página como estando fora da RAM */
        }
    }

    /* Carrega a nova página no quadro que acabamos de conseguir (Swap-in) */
    if (quadro_livre >= 0 && numero_pagina < tabela->max_paginas) {
        /* Atualiza a tabela de páginas para registrar onde a página foi carregada */
        tabela->paginas[numero_pagina].numero_pagina = numero_pagina;
        tabela->paginas[numero_pagina].numero_quadro = quadro_livre;
        tabela->paginas[numero_pagina].valido        = 1;
        tabela->paginas[numero_pagina].tempo_carga   = tempo_atual;
        tabela->paginas[numero_pagina].ultimo_uso    = tempo_atual;

        /* Atualiza as estruturas da memória física para saber quem é o dono deste quadro */
        memoria->quadros[quadro_livre] = tabela->id_processo;
        memoria->quadros_em_uso++;
        tabela->quantidade_paginas++;
    }

    return 1; /* 1 indica que ocorreu Page Fault */
}

/* ------------------------------------------------------------------ */
/* Carga Completa do Processo na Memória                              */
/* ------------------------------------------------------------------ */

int carregar_processo_memoria(MemoriaFisica *memoria, TabelaPaginas *tabela,
                              int id_processo, int memoria_necessaria_mb,
                              PoliticaSubstituicaoPagina politica, int tempo_atual,
                              Processo *todos_processos, int num_processos, int *referencias_futuras) {
    (void)todos_processos;
    (void)num_processos;
    
    tabela->id_processo        = id_processo;
    tabela->quantidade_paginas = 0;
    tabela->dica_vitima        = 0;

    /* Calcula quantas páginas no total este processo usará (1 MB = 1024 KB) */
    int paginas_necessarias = (memoria_necessaria_mb * 1024) / TAMANHO_PAGINA_KB;
    tabela->max_paginas = paginas_necessarias;
    
    /* Aloca a Tabela de Páginas do processo e zera os dados */
    tabela->paginas = (EntradaTabelaPagina *)malloc(paginas_necessarias * sizeof(EntradaTabelaPagina));
    memset(tabela->paginas, 0, paginas_necessarias * sizeof(EntradaTabelaPagina));
    
    int falhas_de_pagina = 0;
    
    /* Geração de string de acessos ("referências") pseudo-aleatórias para simular leituras à memória */
    int tamanho_refs = paginas_necessarias * 3;
    int *referencias = referencias_futuras;
    int liberar_referencias_criadas = 0;
    
    if (referencias == NULL) {
        referencias = (int*)malloc(tamanho_refs * sizeof(int));
        liberar_referencias_criadas = 1;
        srand(id_processo); /* Seed determinística atrelada ao PID do processo para os testes serem reproduzíveis */
        
        for (int i = 0; i < tamanho_refs; i++) {
            if (i < paginas_necessarias) {
                referencias[i] = i; /* Inicializa acessando sequencialmente todas as páginas ao menos uma vez */
            } else {
                referencias[i] = rand() % paginas_necessarias; /* Os acessos subsequentes são em regiões aleatórias da memória do processo */
            }
        }
    } else {
        tamanho_refs = paginas_necessarias; /* Usa o que foi provido */
    }

    /* Simula cada um dos acessos sequencialmente */
    for (int i = 0; i < tamanho_refs; i++) {
        falhas_de_pagina += acessar_pagina_memoria(memoria, tabela, referencias[i], politica, i, referencias, tamanho_refs);
    }
    
    if (liberar_referencias_criadas) {
        free(referencias);
    }

    printf("[MEMORIA] Processo %d simulado: %d páginas necessárias, ocorreu(ram) %d Page Faults.\n",
           id_processo, paginas_necessarias, falhas_de_pagina);
    return falhas_de_pagina;
}

/* ------------------------------------------------------------------ */
/* Liberação de Memória ao Finalizar Processo                         */
/* ------------------------------------------------------------------ */

void liberar_processo_memoria(MemoriaFisica *memoria, TabelaPaginas *tabela, int id_processo) {
    /* Passa por todas as entradas na Tabela de Páginas procurando o que está na RAM */
    for (int i = 0; i < tabela->max_paginas; i++) {
        EntradaTabelaPagina *entrada = &tabela->paginas[i];
        if (entrada->valido) {
            /* Se estiver na memória principal, nós limpamos esse quadro */
            memoria->quadros[entrada->numero_quadro] = -1;
            memoria->quadros_em_uso--;
            entrada->valido = 0;
        }
    }
    
    tabela->quantidade_paginas = 0;
    
    /* Desaloca a estrutura da tabela de páginas que criamos */
    if (tabela->paginas) {
        free(tabela->paginas);
        tabela->paginas = NULL;
    }
    printf("[MEMORIA] Processo %d foi terminado e toda sua memória foi liberada.\n", id_processo);
}

/* ------------------------------------------------------------------ */
/* Emissão de Relatórios Estatísticos                                 */
/* ------------------------------------------------------------------ */

void imprimir_estatisticas_memoria(const MemoriaFisica *memoria, const ResultadoMemoria *resultado) {
    printf("\n=== Relatório de Desempenho da Memória ===\n");
    printf("Total de quadros físicos : %d\n", memoria->total_quadros);
    printf("Quadros atualmente em uso: %d\n", memoria->quadros_em_uso);
    printf("Total de requisições RAM : %d\n", resultado->total_acessos);
    printf("Total de Page Faults     : %d\n", resultado->total_falhas_pagina);
    
    if (resultado->total_acessos > 0) {
        printf("Taxa de falhas (Miss)    : %.2f%%\n",
               100.0 * resultado->total_falhas_pagina / resultado->total_acessos);
    }
}