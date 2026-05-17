#ifndef MEMORY_H
#define MEMORY_H

#include "process.h"

#define PAGE_SIZE_KB   4      /* Tamanho de página em KB */
#define MAX_FRAMES     1024
#define MAX_PAGES      2048

typedef enum {
    PAGE_REPLACE_FIFO,
    PAGE_REPLACE_LRU,
    PAGE_REPLACE_OPTIMAL
} PageReplacePolicy;

typedef struct {
    int page_number;
    int frame_number;
    int valid;        /* 1 = na memória física, 0 = na memória virtual */
    int last_used;    /* timestamp de último acesso (para LRU) */
    int load_time;    /* timestamp de carga (para FIFO) */
} PageTableEntry;

typedef struct {
    int pid;
    PageTableEntry *pages;
    int page_count;
    int max_pages;
    int victim_hint;
} PageTable;

typedef struct {
    int *frames;   /* pid do processo ocupando o frame, -1 = livre */
    int frame_count;          /* total de frames físicos disponíveis */
    int used_frames;
} PhysicalMemory;

typedef struct {
    int total_page_faults;
    int total_accesses;
} MemoryResult;

/* Inicializa memória física e virtual */
void memory_init(PhysicalMemory *mem, int physical_mb, int virtual_mb);

/* Carrega processo na memória; retorna número de page faults gerados */
int  memory_load_process(PhysicalMemory *mem, PageTable *pt,
                         int pid, int memory_needed_mb,
                         PageReplacePolicy policy, int current_time,
                         Process *all_procs, int n_procs, int *future_refs);

/* Remove processo da memória ao terminar */
void memory_free_process(PhysicalMemory *mem, PageTable *pt, int pid);

/* Acessa uma página; retorna 1 se page fault, 0 se hit */
int  memory_access_page(PhysicalMemory *mem, PageTable *pt,
                        int page_number, PageReplacePolicy policy,
                        int current_time,
                        int *future_refs, int future_len);

/* Relatório de uso de memória */
void memory_print_stats(const PhysicalMemory *mem, const MemoryResult *res);

#endif /* MEMORY_H */
