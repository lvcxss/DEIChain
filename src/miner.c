// Código da autoria de Lucas Oliveira (2023219472) e Dinis Silva

#include "deichain.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int cnt = 0;

void *mine(void *idp) {
  int mine_id = *(int *)idp;
  while (1) {
    Block new_block;
    new_block.block_id = mine_id + cnt;
    new_block.timestamp = time(NULL);

    int pipe_fd = open("VALIDATOR_INPUT", O_WRONLY);
    if (pipe_fd == -1) {
      perror("Erro ao abrir o pipe");
      return NULL;
    }
    write(pipe_fd, &new_block, sizeof(Block));
    close(pipe_fd);
  }
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
    char msg[256];
    sprintf(msg, "Miner %d finished", id[i]);
    write_logfile(msg, "INFO");
    printf("%s\n", msg);
  }
}
