#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/process.h"

/* Formato CSV esperado (sem cabeçalho ou com cabeçalho ignorado):
 * pid,nome,arrival_time,burst_time,priority,memory_mb
 * Exemplo:
 *   1,P1,0,8,2,64
 *   2,P2,2,4,1,32
 */
int load_processes_from_csv(const char *filepath, Process procs[], int max) {
    FILE *f = fopen(filepath, "r");
    if (!f) {
        fprintf(stderr, "[ERRO] Não foi possível abrir: %s\n", filepath);
        return -1;
    }

    char line[256];
    int count = 0;

    /* Pula linha de cabeçalho se começar com letra */
    if (fgets(line, sizeof(line), f)) {
        if (line[0] >= 'A' && line[0] <= 'z') {
            /* era cabeçalho, já descartado */
        } else {
            /* era dado, parseia */
            rewind(f);
        }
    }

    while (fgets(line, sizeof(line), f) && count < max) {
        Process *p = &procs[count];
        memset(p, 0, sizeof(Process));

        int parsed = sscanf(line, "%d,%31[^,],%d,%d,%d,%d",
                            &p->pid,
                            p->name,
                            &p->arrival_time,
                            &p->burst_time,
                            &p->priority,
                            &p->memory_needed);
        printf("[DEBUG] PID=%d nome=%s arrival=%d burst=%d priority=%d mem=%d\n",
           p->pid, p->name, p->arrival_time, p->burst_time,
           p->priority, p->memory_needed);

        if (parsed < 6) {
            fprintf(stderr, "[AVISO] Linha ignorada (formato inválido): %s", line);
            continue;
        }

        p->remaining_time = p->burst_time;
        p->state          = STATE_NEW;
        p->start_time     = -1;
        p->finish_time    = -1;
        count++;
    }

    fclose(f);
    return count;
}

void print_processes(const Process procs[], int n) {
    printf("%-5s %-12s %-8s %-8s %-8s %-10s\n",
           "PID", "Nome", "Chegada", "Burst", "Prior.", "Mem(MB)");
    printf("----------------------------------------------------\n");
    for (int i = 0; i < n; i++) {
        const Process *p = &procs[i];
        printf("%-5d %-12s %-8d %-8d %-8d %-10d\n",
               p->pid, p->name, p->arrival_time,
               p->burst_time, p->priority, p->memory_needed);
    }
}
