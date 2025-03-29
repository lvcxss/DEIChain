#include "controller.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

void *mine(void *idp) {
  int mine_id = *(int *)idp;
  char msg[256];
  sprintf(msg, "IM MINING (id : %d)", mine_id);
  write_logfile(msg, "INFO");
  printf("%s\n", msg);
  return NULL;
}

void initminers(int num) {
  pthread_t thread_miners[num];
  int id[num];
  int i;
  char msg[256];
  // creates threads
  for (i = 0; i < num; i++) {
    id[i] = i + 1;
    pthread_create(&thread_miners[i], NULL, mine, &id[i]);
    sprintf(msg, "Miner %d created", id[i]);
    write_logfile(msg, "INIT");
  }

  // waits for threads
  for (i = 0; i < num; i++) {
    pthread_join(thread_miners[i], NULL);
  }
}
