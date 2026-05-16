#ifndef PROCESS_H
#define PROCESS_H

#define MAX_PROCESSES 256
#define MAX_NAME_LEN  32

typedef enum {
    STATE_NEW,
    STATE_READY,
    STATE_RUNNING,
    STATE_WAITING,
    STATE_TERMINATED
} ProcessState;

typedef struct {
    int   pid;
    char  name[MAX_NAME_LEN];
    int   arrival_time;      /* Tempo de chegada */
    int   burst_time;        /* Tempo de execução (CPU Burst) */
    int   remaining_time;    /* Tempo restante (para preempção) */
    int   priority;          /* Prioridade (menor = maior prioridade) */
    int   memory_needed;     /* Memória necessária em MB */
    ProcessState state;

    /* Métricas calculadas ao final */
    int   start_time;
    int   finish_time;
    int   waiting_time;
    int   response_time;
    int   turnaround_time;
} Process;

/* Carrega processos de um arquivo CSV.
 * Retorna o número de processos lidos, ou -1 em erro. */
int  load_processes_from_csv(const char *filepath, Process procs[], int max);

/* Imprime lista de processos (debug) */
void print_processes(const Process procs[], int n);

#endif /* PROCESS_H */
