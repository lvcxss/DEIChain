#include "controller.h"
#include "deichain.h"
#include <stdio.h>

int main() {
  write_logfile("O programa foi iniciado", "INIT");
  printf("O programa foi iniciado");
  init();

  return 0;
}
