/*
 * main.c — Ponto de entrada do Simulador de SO
 *
 * Delega para gui_run() que inicializa o Qt e exibe a janela principal.
 * Para testes sem GUI, descomente o bloco #ifdef CLI_MODE abaixo.
 */

#include <stdio.h>
#include "mainwindow.h"

/* --- Modo CLI para testes rápidos sem Qt (opcional) --------------- */
#ifdef CLI_MODE
#include "process.h"
#include "scheduler.h"
#include "memory.h"
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso (CLI): %s <arquivo.csv>\n", argv[0]);
        return 1;
    }

    Process procs[MAX_PROCESSES];
    int n = load_processes_from_csv(argv[1], procs, MAX_PROCESSES);
    if (n <= 0) return 1;

    printf("=== Processos carregados ===\n");
    print_processes(procs, n);

    SchedulerResult result;
    memset(&result, 0, sizeof(result));
    run_scheduler(procs, n, SCHED_ROUND_ROBIN, 4, &result);

    printf("\n=== Linha do Tempo (Round-Robin, q=4) ===\n");
    for (int i = 0; i < result.count; i++) {
        TimelineEntry *e = &result.entries[i];
        printf("[%2d-%2d] %s\n", e->time_start, e->time_end,
               e->pid == -1 ? "idle" : "P");
    }

    printf("\nEspera Média  : %.2f\n", result.avg_waiting_time);
    printf("Resposta Média: %.2f\n", result.avg_response_time);
    printf("Turnaround M. : %.2f\n", result.avg_turnaround_time);

    PhysicalMemory mem;
    MemoryResult   mres = {0, 0};
    memory_init(&mem, 256, 1024);

    PageTable pt;
    for (int i = 0; i < n; i++) {
        memset(&pt, 0, sizeof(pt));
        int f = memory_load_process(&mem, &pt, procs[i].pid,
                                    procs[i].memory_needed,
                                    PAGE_REPLACE_LRU,
                                    procs[i].arrival_time,
                                    procs, n, NULL);
        mres.total_page_faults += f;
        mres.total_accesses    += (procs[i].memory_needed * 1024) / PAGE_SIZE_KB;
        memory_free_process(&mem, &pt, procs[i].pid);
    }
    memory_print_stats(&mem, &mres);
    return 0;
}
#else
/* --- Modo GUI (padrão) -------------------------------------------- */
int main(int argc, char *argv[]) {
    return gui_run(argc, argv);
}
#endif
