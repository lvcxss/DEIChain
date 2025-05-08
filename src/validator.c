// Código da autoria de Lucas Oliveira (2023219472) e Dinis Silva
#include "deichain.h"
#include "pow.h"
#include <errno.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include <signal.h>
#include <stdio.h>
void validator() {
  write_logfile("Validator process", "INIT");
  printf("IM VALIDATING\n");
}
