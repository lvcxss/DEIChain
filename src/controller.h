#ifndef CONTROLLER_H
#define CONTROLLER_H
#include "deichain.h"
#include "transaction.h"
#include <semaphore.h>
void init();
void write_logfile(char *message, char *typemsg);
Config processFile(char *filename);

#endif
