//

#include "statistics.h"
#include "deichain.h"
#include <pthread.h>
#include <stdio.h>
#include <sys/shm.h>

int write_logfile(char *message, char *typemsg);
void print_statistics() {
  write_logfile("im statistics!!!", "INFO");
  printf("im statistics!!!\n");
  exit(0);
}
