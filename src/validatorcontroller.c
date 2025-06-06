#include "deichain.h"
#include "validator.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_VALIDATORS 3
int write_logfile(char *message, char *typemsg);

volatile sig_atomic_t vc_exit = 0;
pid_t validator_pids[MAX_VALIDATORS] = {0};
int current_validators = 0;

// signal handlers
void handle_sigtermvc(int sig) {
  (void)sig;
  vc_exit = 1;
  pthread_cond_broadcast(&transactions_pool->cond_vc);
  for (int i = 0; i < MAX_VALIDATORS; i++) {
    if (validator_pids[i] > 0) {
      kill(validator_pids[i], SIGTERM);
    }
  }
}

// spawning a validator proccess
int spawn_validator(int index) {
  pid_t pid = fork();
  if (pid < 0) {
    perror("Erro ao criar validator");
    return -1;
  } else if (pid == 0) {
    validator_id = index + 1;
    char msg[64];
    sprintf(msg, "Validator %d criado", validator_id);
    write_logfile(msg, "Validator Controller");
    int ret = validator();
    _exit(ret);

  } else {
    validator_pids[index] = pid;
    current_validators++;
    printf("[ValidatorController] Validator %d criado (PID %d)\n", index + 1,
           pid);
    return 0;
  }
}

// killing a validator proccess
void kill_validator(int index) {
  if (validator_pids[index] > 0) {
    kill(validator_pids[index], SIGTERM);
    waitpid(validator_pids[index], NULL, 0);
    printf("[ValidatorController] Validator %d terminado (PID %d)\n", index,
           validator_pids[index]);
    validator_pids[index] = 0;
    current_validators--;
  }
}

int validator_controller() {
  struct sigaction sa;
  sa.sa_handler = handle_sigtermvc;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGTERM, &sa, NULL);

  pthread_mutex_lock(&transactions_pool->mt_vc);
  while (!vc_exit) {
    sem_wait(transactions_pool->tp_access_pool);
    // count number of occupied and check if more validators are needed
    float ocupacao = 0;
    for (unsigned int itt = 0; itt < transactions_pool->atual; itt++) {
      if (transactions[itt].occupied) {
        ocupacao++;
      }
    }
    // get the % of occupied
    ocupacao *= 100.0f;
    ocupacao = ocupacao / transactions_pool->max_size;
    sem_post(transactions_pool->tp_access_pool);
    int desired_validators = 1;
    // check if any more are needed
    if (ocupacao >= 80.0f)
      desired_validators = 3;
    else if (ocupacao >= 60.0f)
      desired_validators = 2;

    if (vc_exit)
      break;
    for (int i = 0; i < desired_validators; i++) {
      if (validator_pids[i] == 0) {
        spawn_validator(i);
      }
    }

    if (vc_exit)
      break;
    for (int i = desired_validators; i < MAX_VALIDATORS; i++) {
      if (validator_pids[i] > 0) {
        kill_validator(i);
      }
    }

    if (vc_exit)
      break;
    pthread_cond_wait(&transactions_pool->cond_vc, &transactions_pool->mt_vc);
  }
  pthread_mutex_unlock(&transactions_pool->mt_vc);

  for (int i = 0; i < MAX_VALIDATORS; i++) {
    if (validator_pids[i] > 0) {
      kill_validator(i);
    }
  }
  // destroy here the mutex and cond to avoid deadlocks
  pthread_cond_destroy(&transactions_pool->cond_vc);
  pthread_mutex_destroy(&transactions_pool->mt_vc);
  write_logfile("Validator Controller terminado", "Validator Controller");
  printf("[ValidatorController] Terminado com sucesso.\n");
  return 0;
}
