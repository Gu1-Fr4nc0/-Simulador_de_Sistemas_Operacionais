#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/scheduler.h"

/* ------------------------------------------------------------------ */
/* Utilitários internos                                                 */
/* ------------------------------------------------------------------ */

static void add_timeline(SchedulerResult *r, int pid, int t_start, int t_end) {
    if (r->count >= 4096) return;
    r->entries[r->count].pid        = pid;
    r->entries[r->count].time_start = t_start;
    r->entries[r->count].time_end   = t_end;
    r->count++;
}

void compute_metrics(Process procs[], int n, SchedulerResult *r) {
    double total_wait = 0, total_resp = 0, total_turn = 0;
    for (int i = 0; i < n; i++) {
        Process *p = &procs[i];
        p->turnaround_time = p->finish_time - p->arrival_time;
        p->waiting_time    = p->turnaround_time - p->burst_time;
        p->response_time   = p->start_time - p->arrival_time;

        total_wait += p->waiting_time;
        total_resp += p->response_time;
        total_turn += p->turnaround_time;
    }
    r->avg_waiting_time    = total_wait / n;
    r->avg_response_time   = total_resp / n;
    r->avg_turnaround_time = total_turn / n;
}


/* ------------------------------------------------------------------ */
/* Round-Robin                                                          */
/* ------------------------------------------------------------------ */

void sched_round_robin(Process procs[], int n, int quantum, SchedulerResult *r) {
    /* Copia local para não modificar o array original */
    Process ps[MAX_PROCESSES];
    memcpy(ps, procs, n * sizeof(Process));

    int queue[MAX_PROCESSES * 100];
    int head = 0, tail = 0;
    int time = 0;
    int completed = 0;

    /* Fila circular de índices */
    #define ENQUEUE(idx) queue[tail++ % (MAX_PROCESSES*100)] = (idx)
    #define DEQUEUE()    queue[head++ % (MAX_PROCESSES*100)]

    /* Adiciona processos que chegam no tempo 0 */
    for (int i = 0; i < n; i++)
        if (ps[i].arrival_time == 0) ENQUEUE(i);

    while (completed < n) {
        if (head == tail) { /* fila vazia, CPU ociosa */
            add_timeline(r, -1, time, time + 1);
            time++;
            /* verifica chegadas */
            for (int i = 0; i < n; i++)
                if (ps[i].arrival_time == time && ps[i].state == STATE_NEW)
                    ENQUEUE(i);
            continue;
        }

        int idx = DEQUEUE();
        Process *p = &ps[idx];

        if (p->state == STATE_NEW || p->state == STATE_READY) {
            if (p->start_time == -1) p->start_time = time;
            p->state = STATE_RUNNING;
        }

        int run = (p->remaining_time < quantum) ? p->remaining_time : quantum;
        add_timeline(r, p->pid, time, time + run);
        p->remaining_time -= run;
        time += run;

        /* Verifica chegadas durante o quantum */
        for (int i = 0; i < n; i++)
            if (ps[i].arrival_time > time - run &&
                ps[i].arrival_time <= time &&
                ps[i].state == STATE_NEW) {
                ps[i].state = STATE_READY;
                ENQUEUE(i);
            }

        if (p->remaining_time == 0) {
            p->state       = STATE_TERMINATED;
            p->finish_time = time;
            completed++;
        } else {
            p->state = STATE_READY;
            ENQUEUE(idx);
        }
    }

    /* Copia métricas de volta */
    memcpy(procs, ps, n * sizeof(Process));
    compute_metrics(procs, n, r);
    #undef ENQUEUE
    #undef DEQUEUE
}

/* ------------------------------------------------------------------ */
/* SJF Preemptivo (Shortest Remaining Time First - SRTF)               */
/* ------------------------------------------------------------------ */

void sched_sjf_preemptive(Process procs[], int n, SchedulerResult *r) {
    Process ps[MAX_PROCESSES];
    memcpy(ps, procs, n * sizeof(Process));

    int time = 0, completed = 0;
    int prev_pid = -1, slice_start = 0;

    while (completed < n) {
        /* Encontra processo com menor remaining_time entre os chegados */
        int best = -1;
        for (int i = 0; i < n; i++) {
            if (ps[i].arrival_time <= time && ps[i].state != STATE_TERMINATED) {
                if (best == -1 || ps[i].remaining_time < ps[best].remaining_time)
                    best = i;
            }
        }

        if (best == -1) { /* CPU ociosa */
            if (prev_pid != -1) {
                add_timeline(r, prev_pid, slice_start, time);
                prev_pid = -1;
            }
            add_timeline(r, -1, time, time + 1);
            time++;
            continue;
        }

        Process *p = &ps[best];
        if (p->start_time == -1) p->start_time = time;
        if (p->state != STATE_RUNNING) p->state = STATE_RUNNING;

        /* Detecta troca de processo para compactar timeline */
        if (p->pid != prev_pid) {
            if (prev_pid != -1)
                add_timeline(r, prev_pid, slice_start, time);
            slice_start = time;
            prev_pid = p->pid;
        }

        p->remaining_time--;
        time++;

        if (p->remaining_time == 0) {
            p->state       = STATE_TERMINATED;
            p->finish_time = time;
            completed++;
            add_timeline(r, p->pid, slice_start, time);
            prev_pid   = -1;
            slice_start = time;
        }
    }

    memcpy(procs, ps, n * sizeof(Process));
    compute_metrics(procs, n, r);
}

/* ------------------------------------------------------------------ */
/* Prioridade Preemptiva                                                */
/* ------------------------------------------------------------------ */

void sched_priority_preemptive(Process procs[], int n, SchedulerResult *r) {
    Process ps[MAX_PROCESSES];
    memcpy(ps, procs, n * sizeof(Process));



    int time = 0, completed = 0;
    int prev_pid = -1, slice_start = 0;

    while (completed < n) {
        /* Maior prioridade = menor número de prioridade */
        int best = -1;
        for (int i = 0; i < n; i++) {
            if (ps[i].arrival_time <= time && ps[i].state != STATE_TERMINATED) {
                if (best == -1 || ps[i].priority < ps[best].priority)
                    best = i;
            }
        }

        if (best == -1) {
            if (prev_pid != -1) {
                add_timeline(r, prev_pid, slice_start, time);
                prev_pid = -1;
            }
            add_timeline(r, -1, time, time + 1);
            time++;
            continue;
        }

        Process *p = &ps[best];
        if (p->start_time == -1) p->start_time = time;
        p->state = STATE_RUNNING;

        if (p->pid != prev_pid) {
            if (prev_pid != -1)
                add_timeline(r, prev_pid, slice_start, time);
            slice_start = time;
            prev_pid = p->pid;
        }

        p->remaining_time--;
        time++;


        if (p->remaining_time == 0) {
            p->state       = STATE_TERMINATED;
            p->finish_time = time;
            completed++;
            add_timeline(r, p->pid, slice_start, time);
            prev_pid    = -1;
            slice_start = time;
        }
    }

    memcpy(procs, ps, n * sizeof(Process));
    compute_metrics(procs, n, r);
}

/* ------------------------------------------------------------------ */
/* Dispatcher                                                           */
/* ------------------------------------------------------------------ */

void run_scheduler(Process procs[], int n,
                   SchedulerType type, int quantum,
                   SchedulerResult *result) {
    memset(result, 0, sizeof(SchedulerResult));
    switch (type) {
        case SCHED_ROUND_ROBIN:
            sched_round_robin(procs, n, quantum, result);
            break;
        case SCHED_SJF_PREEMPTIVE:
            sched_sjf_preemptive(procs, n, result);
            break;
        case SCHED_PRIORITY_PREEMPTIVE:
            sched_priority_preemptive(procs, n, result);
            break;
    }
}
