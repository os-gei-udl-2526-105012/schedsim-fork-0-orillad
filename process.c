#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "process.h"

const char* processStatusNames[] = {"Ready", "Running", "Bloqued","Finished"};

Process initProcess(int id, char* name, int burst, int priority, int arrive_time){
    Process p;
    p.id = id;
    p.name=strdup(name);
    p.burst=burst;
    p.priority=priority;
    p.arrive_time=arrive_time;
    return p;
}

Process initProcessFromTokens(char* line, char* separator){

  const size_t BUFSIZE = 32;   
  size_t bufsize = BUFSIZE;
  Process p;

  char *feature = malloc(bufsize * sizeof(char));
  feature=strtok(line, separator); 

  int features = 0;
  while (feature != NULL) {
            switch (features)
            {
            case 0:
                p.id=atoi(feature);
                break;
            case 1:
                p.name=malloc(sizeof(feature));
                strcpy(p.name,feature);
                break;
            case 2:
                p.priority=atoi(feature);
                break;
            case 3:
                p.arrive_time=atoi(feature);
                break;
            case 4:
                p.burst=atoi(feature);
            }
            feature=strtok(NULL, ";"); 
            features++;
    }
  free(feature);
  return p;
}

void destroyProcess(Process p){

    if (p.name != NULL){
        free(p.name);
    }

    if (p.lifecycle != NULL){
        free(p.lifecycle);
    }
};


void printProcess(Process proces){
    printf("{id:%d, name:%s, priority:%d, arrive_time:%d, burst:%d}\n", proces.id,proces.name,proces.priority,proces.arrive_time,proces.burst);
}

int compareArrival(const void *s1, const void *s2){
    Process *p1 = (Process *)s1;
    Process *p2 = (Process *)s2;
    if (p1->arrive_time > p2->arrive_time) 
        return 1; 
    else if (p1->arrive_time < p2->arrive_time)  
        return -1;
    else
        return 0;
}

int compareBurst(const void *s1, const void *s2){
    Process *p1 = (Process *)s1;
    Process *p2 = (Process *)s2;
    if (p1->burst > p2->burst){
        return 1; 
    } else if (p1->burst < p2->burst){
        return -1;
    }
    /* Empats: primer qui arriba abans, després id menor */
    if (p1->arrive_time > p2->arrive_time){
        return 1;
    } else if (p1->arrive_time < p2->arrive_time){
        return -1;
    }
    return (p1->id > p2->id) ? 1 : (p1->id < p2->id ? -1 : 0);
}

int comparePriority(const void *s1, const void *s2){
    Process *p1 = (Process *)s1;
    Process *p2 = (Process *)s2;
    if (p1->priority > p2->priority){ 
        return 1; 
    } else if (p1->priority < p2->priority){  
        return -1;
    }
    /* Empats: primer qui arriba abans, després id menor */
    if (p1->arrive_time > p2->arrive_time){
        return 1;
    } else if (p1->arrive_time < p2->arrive_time){
        return -1;
    }
    return (p1->id > p2->id) ? 1 : (p1->id < p2->id ? -1 : 0);
}




