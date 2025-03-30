// Código da autoria de Lucas Oliveira (2023219472) e Dinis Silva
// Créditos: slides e demos das aulas TP
#include "statistics.h"
#include "deichain.h"
#include <stdio.h>

int write_logfile(char *message, char *typemsg);

void print_statistics() {
  write_logfile("im statistics!!!", "INFO");
  printf("im statistics!!!\n");
}
