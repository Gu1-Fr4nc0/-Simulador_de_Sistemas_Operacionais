#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

typedef enum {
    SCHED_ROUND_ROBIN,
    SCHED_SJF_PREEMPTIVE,
    SCHED_PRIORITY_PREEMPTIVE
} SchedulerType;

/* Entrada de linha do tempo gerada pelo escalonador */
typedef struct {
    int pid;          /* -1 = CPU ociosa */
    int time_start;
    int time_end;
} TimelineEntry;

typedef struct {
    TimelineEntry entries[4096];
    int           count;
    double        avg_waiting_time;
    double        avg_response_time;
    double        avg_turnaround_time;
} SchedulerResult;

/* Executa a simulação de escalonamento.
 * procs[]  : array de processos (será modificado internamente com cópias)
 * n        : número de processos
 * type     : algoritmo escolhido
 * quantum  : usado somente para Round-Robin
 * result   : estrutura preenchida com linha do tempo e métricas
 */
void run_scheduler(Process procs[], int n,
                   SchedulerType type, int quantum,
                   SchedulerResult *result);

/* --- Algoritmos individuais (implementar em scheduler.c) --- */
void sched_round_robin      (Process procs[], int n, int quantum,  SchedulerResult *r);
void sched_sjf_preemptive   (Process procs[], int n,               SchedulerResult *r);
void sched_priority_preemptive(Process procs[], int n,             SchedulerResult *r);

/* Calcula métricas agregadas a partir do array de processos finalizado */
void compute_metrics(Process procs[], int n, SchedulerResult *r);

#endif /* SCHEDULER_H */
