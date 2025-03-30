#include "transaction.h"
#include "deichain.h"
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int write_logfile(char *message, char *typemsg);

void signal_handler(int signum) {
  if (signum == SIGINT) {
    char msg[256];
    fprintf(stderr, "Killing process \n");
    sprintf(msg, "SIGINT received on transaction generator with id: %d.",
            getpid());
    write_logfile(msg, "INFO");
    exit(0);
  }
}

int main(int argc, char *argv[]) {
  signal(SIGINT, signal_handler);
  if (argc != 3) {
    printf("Usage: %s <reward> <sleep time>\n", argv[0]);
    return 1;
  }

  int reward = atoi(argv[1]);
  if (reward == 0 && argv[1][0] != '0') {
    printf("Reward must be a number\n");
    exit(1);
  }
  int sleepTime = atoi(argv[2]);
  if (sleepTime == 0 && argv[2][0] != '0') {
    printf("Sleep time must be a number\n");
    exit(1);
  }

  if (reward > 3 || reward < 1) {
    printf("Reward must be between 1 and 3\n");
    exit(1);
  }

  if (sleepTime > 3000 || sleepTime < 200) {
    printf("Sleep time must be between 200 and 3000\n");
    exit(1);
  }
  char msg[256];
  Transaction t;
  t = create_transaction(reward, 4);
  sprintf(msg, "Transaction with reward %d and sleep time %d pid: %d created",
          t.reward, sleepTime, getpid());
  printf("%s\n", msg);
  write_logfile(msg, "INFO");
  return 0;
}
