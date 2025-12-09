#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "process.h"


# define EMPTY -1
# define NOT_INIT 0

void init_queue(void);
int enqueue(Process* item);
Process* dequeue(void);
Process* peek(void);
size_t get_queue_size(void);
void cleanQueue(void);
char* queueToString(void);
Process* transformQueueToList(void);
void setQueueFromList(Process* list);

#endif
