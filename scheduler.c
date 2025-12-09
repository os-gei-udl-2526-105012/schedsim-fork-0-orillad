#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "process.h"
#include "queue.h"
#include "scheduler.h"

int num_algorithms() {
  return sizeof(algorithmsNames) / sizeof(char *);
}

int num_modalities() {
  return sizeof(modalitiesNames) / sizeof(char *);
}

size_t initFromCSVFile(char* filename, Process** procTable){
    FILE* f = fopen(filename,"r");
    
    size_t procTableSize = 10;
    
    *procTable = malloc(procTableSize * sizeof(Process));
    Process * _procTable = *procTable;

    if(f == NULL){
      perror("initFromCSVFile():::Error Opening File:::");   
      exit(1);             
    }

    char* line = NULL;
    size_t buffer_size = 0;
    size_t nprocs= 0;
    while( getline(&line,&buffer_size,f)!=-1){
        if(line != NULL){
            Process p = initProcessFromTokens(line,";");

            if (nprocs==procTableSize-1){
                procTableSize=procTableSize+procTableSize;
                _procTable=realloc(_procTable, procTableSize * sizeof(Process));
            }

            _procTable[nprocs]=p;

            nprocs++;
        }
    }
   free(line);
   fclose(f);
   return nprocs;
}

size_t getTotalCPU(Process *procTable, size_t nprocs){
    size_t total=0;
    for (int p=0; p<nprocs; p++ ){
        total += (size_t) procTable[p].burst;
    }
    return total;
}

int getCurrentBurst(Process* proc, int current_time){
    int burst = 0;
    for(int t=0; t<current_time; t++){
        if(proc->lifecycle[t] == Running){
            burst++;
        }
    }
    return burst;
}

int run_dispatcher(Process *procTable, size_t nprocs, int algorithm, int modality, int quantum){

    Process *current = NULL;
    int q_used = 0;
    size_t completed = 0;
    size_t current_time = 0;

    qsort(procTable,nprocs,sizeof(Process),compareArrival);

    init_queue();

    /* reservem espai suficient per al lifecycle (arribades tardanes + CPU total) */
    int max_arrive = 0;
    for (size_t p = 0; p < nprocs; p++){
        if (procTable[p].arrive_time > max_arrive){
            max_arrive = procTable[p].arrive_time;
        }
    }
    size_t duration = getTotalCPU(procTable, nprocs) + (size_t)max_arrive + 1;

    for (int p=0; p<nprocs; p++ ){
        procTable[p].lifecycle = malloc( duration * sizeof(int));
        for(size_t t=0; t<duration; t++){
            procTable[p].lifecycle[t]=-1;
        }
        procTable[p].waiting_time = 0;
        procTable[p].return_time = 0;
        procTable[p].response_time = -1;
        procTable[p].completed = false;
    }

    while (completed < nprocs && current_time < duration){
        /* Arribades en aquest tick */
        for (size_t p = 0; p < nprocs; p++){
            if ((size_t)procTable[p].arrive_time == current_time){
                enqueue(&procTable[p]);
                procTable[p].lifecycle[current_time] = Ready;
            }
        }

        /* Reordena cua si cal segons l'algorisme */
        if (get_queue_size() > 1){
            int need_sort = 0;
            if (modality == PREEMPTIVE && (algorithm == SJF || algorithm == PRIORITIES)){
                need_sort = 1;
            } else if (current == NULL && modality == NONPREEMPTIVE && (algorithm == SJF || algorithm == PRIORITIES)){
                need_sort = 1;
            }
            if (need_sort){
                Process* list = transformQueueToList();
                size_t qsize = get_queue_size();
                if (algorithm == SJF){
                    qsort(list, qsize, sizeof(Process), compareBurst);
                } else if (algorithm == PRIORITIES){
                    qsort(list, qsize, sizeof(Process), comparePriority);
                }
                setQueueFromList(list);
                free(list);
            }
        }

        /* Seleccionar procés si la CPU és lliure */
        if (current == NULL){
            current = dequeue();
            q_used = 0;
            if (current != NULL && current->response_time == -1){
                current->response_time = (int)current_time - current->arrive_time;
            }
        }

        if (current != NULL){
            current->lifecycle[current_time] = Running;
        }

        /* Marcar estats per a la resta de processos en aquest tick */
        for (size_t p = 0; p < nprocs; p++){
            if (&procTable[p] == current){
                continue;
            }
            if (procTable[p].completed){
                procTable[p].lifecycle[current_time] = Finished;
            } else if ((size_t)procTable[p].arrive_time <= current_time){
                procTable[p].lifecycle[current_time] = Ready;
            } else {
                procTable[p].lifecycle[current_time] = -1;
            }
        }

        /* Executar un tick de CPU */
        if (current != NULL){
            q_used++;
            int burst_consumed = getCurrentBurst(current, (int)current_time) + 1;
            if (burst_consumed >= current->burst){
                current->completed = true;
                current->return_time = (int)current_time + 1 - current->arrive_time;
                current->lifecycle[current_time] = Finished;
                completed++;
                current = NULL;
                q_used = 0;
            }
        }

        /* Actualitzar temps d'espera */
        for (size_t p = 0; p < nprocs; p++){
            if (&procTable[p] == current){
                continue;
            }
            if (!procTable[p].completed && (size_t)procTable[p].arrive_time <= current_time){
                procTable[p].waiting_time++;
            }
        }

        /* Preempció o rotació segons algorisme */
        if (modality == PREEMPTIVE && current != NULL){
            if (algorithm == RR){
                if (q_used >= quantum){
                    enqueue(current);
                    current = NULL;
                    q_used = 0;
                }
            } else if (algorithm == SJF){
                Process* candidate = peek();
                if (candidate != NULL){
                    int current_remaining = current->burst - getCurrentBurst(current, (int)current_time+1);
                    int cand_remaining = candidate->burst - getCurrentBurst(candidate, (int)current_time);
                    if (cand_remaining < current_remaining){
                        enqueue(current);
                        current = NULL;
                        q_used = 0;
                    }
                }
            } else if (algorithm == PRIORITIES){
                Process* candidate = peek();
                if (candidate != NULL && comparePriority(candidate, current) < 0){
                    enqueue(current);
                    current = NULL;
                    q_used = 0;
                }
            }
        }

        current_time++;
    }

    printSimulation(nprocs,procTable,current_time);
    printMetrics(current_time, nprocs, procTable);

    for (int p=0; p<nprocs; p++ ){
        destroyProcess(procTable[p]);
    }

    cleanQueue();
    return EXIT_SUCCESS;

}


void printSimulation(size_t nprocs, Process *procTable, size_t duration){

    printf("%14s","== SIMULATION ");
    for (int t=0; t<duration; t++ ){
        printf("%5s","=====");
    }
    printf("\n");

    printf ("|%4s", "name");
    for(int t=0; t<duration; t++){
        printf ("|%2d", t);
    }
    printf ("|\n");

    for (int p=0; p<nprocs; p++ ){
        Process current = procTable[p];
            printf ("|%4s", current.name);
            for(int t=0; t<duration; t++){
                printf("|%2s",  (current.lifecycle[t]==Running ? "E" : 
                        current.lifecycle[t]==Bloqued ? "B" :   
                        current.lifecycle[t]==Finished ? "F" : " "));
            }
            printf ("|\n");
        
    }


}

void printMetrics(size_t simulationCPUTime, size_t nprocs, Process *procTable ){

    printf("%-14s","== METRICS ");
    for (int t=0; t<simulationCPUTime+1; t++ ){
        printf("%5s","=====");
    }
    printf("\n");

    printf("= Duration: %ld\n", simulationCPUTime );
    printf("= Processes: %ld\n", nprocs );

    size_t baselineCPUTime = getTotalCPU(procTable, nprocs);
    double throughput = (double) nprocs / (double) simulationCPUTime;
    double cpu_usage = (double) simulationCPUTime / (double) baselineCPUTime;

    printf("= CPU (Usage): %lf\n", cpu_usage*100 );
    printf("= Throughput: %lf\n", throughput*100 );

    double averageWaitingTime = 0;
    double averageResponseTime = 0;
    double averageReturnTime = 0;
    double averageReturnTimeN = 0;

    for (int p=0; p<nprocs; p++ ){
            averageWaitingTime += procTable[p].waiting_time;
            averageResponseTime += procTable[p].response_time;
            averageReturnTime += procTable[p].return_time;
            averageReturnTimeN += procTable[p].return_time / (double) procTable[p].burst;
    }


    printf("= averageWaitingTime: %lf\n", (averageWaitingTime/(double) nprocs) );
    printf("= averageResponseTime: %lf\n", (averageResponseTime/(double) nprocs) );
    printf("= averageReturnTimeN: %lf\n", (averageReturnTimeN/(double) nprocs) );
    printf("= averageReturnTime: %lf\n", (averageReturnTime/(double) nprocs) );

}
