#ifndef CONTROLLER_H
#define CONTROLLER_H
#include "transaction.h"
#include <semaphore.h>

extern int shmid;
extern int shmidtransactions;
extern int *transactionid;
extern sem_t *logfilesem;
extern pthread_mutex_t transactionidmutex;
extern transaction transcations_pool;
void init();
void processFile(unsigned int **arr, char *filename);

#endif
