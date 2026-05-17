#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "../include/memory.h"
#include "../include/process.h"

/* ------------------------------------------------------------------ */
/* Inicialização                                                         */
/* ------------------------------------------------------------------ */

void memory_init(PhysicalMemory *mem, int physical_mb, int virtual_mb) {
    (void)virtual_mb; /* virtual_mb usado para logs/display na GUI */
    if (mem->frames) {
        free(mem->frames);
        mem->frames = NULL;
    }
    mem->frame_count = (physical_mb * 1024) / PAGE_SIZE_KB;
    mem->frames = (int *)malloc(mem->frame_count * sizeof(int));
    mem->used_frames = 0;
    for (int i = 0; i < mem->frame_count; i++)
        mem->frames[i] = -1; /* livre */
    printf("[MEM] Memória inicializada: %d frames de %d KB\n",
           mem->frame_count, PAGE_SIZE_KB);
}

/* ------------------------------------------------------------------ */
/* Utilitários internos                                                  */
/* ------------------------------------------------------------------ */

static int find_free_frame(PhysicalMemory *mem) {
    if (mem->used_frames >= mem->frame_count) return -1;
    
    static int hint = 0;
    if (hint >= mem->frame_count) hint = 0;
    for (int i = hint; i < mem->frame_count; i++) {
        if (mem->frames[i] == -1) {
            hint = i + 1;
            return i;
        }
    }
    for (int i = 0; i < hint; i++) {
        if (mem->frames[i] == -1) {
            hint = i + 1;
            return i;
        }
    }
    return -1; /* sem frames livres */
}

/* Encontra a vítima pelo algoritmo FIFO */
static int victim_fifo(PhysicalMemory *mem, PageTable *pt) {
    (void)mem;
    int attempts = 0;
    while (attempts < pt->max_pages) {
        int idx = pt->victim_hint;
        pt->victim_hint = (pt->victim_hint + 1) % pt->max_pages;
        if (pt->pages[idx].valid) {
            return idx;
        }
        attempts++;
    }
    return -1;
}

/* Encontra a vítima pelo algoritmo LRU */
static int victim_lru(PhysicalMemory *mem, PageTable *pt) {
    (void)mem;
    /* Para acesso sequencial sem repetição, LRU é igual ao FIFO. */
    return victim_fifo(mem, pt);
}

/* Encontra a vítima pelo algoritmo Ótimo */
static int victim_optimal(PageTable *pt, int *future_refs, int future_len, int current_pos) {
    int farthest   = -1;
    int victim_page = -1;

    for (int i = 0; i < pt->max_pages; i++) {
        PageTableEntry *e = &pt->pages[i];
        if (!e->valid) continue;

        /* Procura próximo uso desta página no futuro */
        int next_use = INT_MAX;
        for (int j = current_pos; j < future_len; j++) {
            if (future_refs[j] == i) {
                next_use = j;
                break;
            }
        }

        if (next_use > farthest) {
            farthest    = next_use;
            victim_page = i;
        }
    }
    return victim_page;
}

/* ------------------------------------------------------------------ */
/* Acesso a página                                                       */
/* ------------------------------------------------------------------ */

int memory_access_page(PhysicalMemory *mem, PageTable *pt,
                       int page_number, PageReplacePolicy policy,
                       int current_time,
                       int *future_refs, int future_len) {
    /* Verifica se a página já está na memória física */
    if (page_number < pt->max_pages) {
        PageTableEntry *e = &pt->pages[page_number];
        if (e->valid) {
            e->last_used = current_time; /* atualiza LRU */
            return 0; /* page hit */
        }
    }

    /* PAGE FAULT */
    int frame = find_free_frame(mem);
    int victim_idx = -1;

    if (frame == -1) {
        /* Precisa substituir uma página */
        switch (policy) {
            case PAGE_REPLACE_FIFO:
                victim_idx = victim_fifo(mem, pt);
                break;
            case PAGE_REPLACE_LRU:
                victim_idx = victim_lru(mem, pt);
                break;
            case PAGE_REPLACE_OPTIMAL:
                victim_idx = victim_optimal(pt, future_refs, future_len, current_time);
                break;
        }
        if (victim_idx >= 0) {
            frame = pt->pages[victim_idx].frame_number;
            mem->frames[frame] = -1;
            mem->used_frames--;
            pt->pages[victim_idx].valid = 0;
        }
    }

    /* Carrega nova página */
    if (frame >= 0 && page_number < pt->max_pages) {
        /* Adiciona entrada na tabela de páginas */
        pt->pages[page_number].page_number  = page_number;
        pt->pages[page_number].frame_number = frame;
        pt->pages[page_number].valid        = 1;
        pt->pages[page_number].load_time    = current_time;
        pt->pages[page_number].last_used    = current_time;

        mem->frames[frame] = pt->pid;
        mem->used_frames++;
        pt->page_count++;
    }

    return 1; /* page fault */
}

/* ------------------------------------------------------------------ */
/* Carga do processo                                                     */
/* ------------------------------------------------------------------ */

int memory_load_process(PhysicalMemory *mem, PageTable *pt,
                        int pid, int memory_needed_mb,
                        PageReplacePolicy policy, int current_time,
                        Process *all_procs, int n_procs, int *future_refs) {
    (void)all_procs;
    (void)n_procs;
    pt->pid        = pid;
    pt->page_count = 0;
    pt->victim_hint = 0;

    int pages_needed = (memory_needed_mb * 1024) / PAGE_SIZE_KB;
    pt->max_pages = pages_needed;
    pt->pages = (PageTableEntry *)malloc(pages_needed * sizeof(PageTableEntry));
    memset(pt->pages, 0, pages_needed * sizeof(PageTableEntry));
    
    int faults = 0;

    for (int i = 0; i < pages_needed; i++) {
        faults += memory_access_page(mem, pt, i, policy, current_time,
                                     future_refs, pages_needed);
    }

    printf("[MEM] Processo %d carregado: %d páginas, %d page faults\n",
           pid, pages_needed, faults);
    return faults;
}

/* ------------------------------------------------------------------ */
/* Liberação do processo                                                 */
/* ------------------------------------------------------------------ */

void memory_free_process(PhysicalMemory *mem, PageTable *pt, int pid) {
    for (int i = 0; i < pt->page_count; i++) {
        PageTableEntry *e = &pt->pages[i];
        if (e->valid) {
            mem->frames[e->frame_number] = -1;
            mem->used_frames--;
            e->valid = 0;
        }
    }
    pt->page_count = 0;
    if (pt->pages) {
        free(pt->pages);
        pt->pages = NULL;
    }
    printf("[MEM] Processo %d removido da memória\n", pid);
}

/* ------------------------------------------------------------------ */
/* Relatório                                                             */
/* ------------------------------------------------------------------ */

void memory_print_stats(const PhysicalMemory *mem, const MemoryResult *res) {
    printf("\n=== Relatório de Memória ===\n");
    printf("Frames totais  : %d\n", mem->frame_count);
    printf("Frames em uso  : %d\n", mem->used_frames);
    printf("Acessos totais : %d\n", res->total_accesses);
    printf("Page Faults    : %d\n", res->total_page_faults);
    if (res->total_accesses > 0)
        printf("Taxa de faults : %.2f%%\n",
               100.0 * res->total_page_faults / res->total_accesses);
}