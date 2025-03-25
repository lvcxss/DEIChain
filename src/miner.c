#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
void *mine(void *idp) {
  int mine_id = *(int *)idp;
  printf("IM MINING (id : %d) \n", mine_id);
  return NULL;
}

void initminers(int num) {
  pthread_t thread_miners[num];
  int id[num];
  int i;
  // creates threads
  for (i = 0; i < num; i++) {
    id[i] = i + 1;
    pthread_create(&thread_miners[i], NULL, mine, &id[i]);
  }

  // waits for threads
  for (i = 0; i < num; i++) {
    pthread_join(thread_miners[i], NULL);
  }
}
