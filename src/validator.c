// Código da autoria de Lucas Oliveira (2023219472) e Dinis Silva
#include "deichain.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
 
int write_statistics();
//verifying pow
//verifying hash 
// ageing transactions
void validator() {
  //index acredito que tenha de passar para memoria partilhada
  //para evitar 
  int validated_blocks,invalidated_blocks,index = 0;
  int fd;
  Block received_block;
  if ((fd = open("VALIDATOR_INPUT",O_RDONLY))<0)
    {
      perror("Cannot open pipe:");
      return 1;
    }
    read(fd,&received_block,sizeof(Block)+ sizeof(Transaction) * config.transactions_per_block);
    close(fd);
    Transaction * block_transactions = received_block.transactions; // casting of the transactions in block
    TransactionPoolEntry * tx_pool_entries = (TransactionPoolEntry *)((char*) transactions_pool); 
    sem_wait(&(block_ledger->ledger_sem));
    if(strcmp (block_ledger->blocks[index].previous_hash,received_block.previous_hash )!= 0)
      write_statistics("Invalid block detected", "ERROR");
      sem_post(&(block_ledger->ledger_sem));
      return 1;
    int tx_idx [config.transactions_per_block];
    for(int i = 0; i<config.transactions_per_block;i++)
    {
      for (int j = 0;j<config.tx_pool_size;) {

        if(block_transactions[i].transaction_id != tx_pool_entries->transaction.transaction_id)
        {
          tx_pool_entries->age++;
          if ((tx_pool_entries->age%50)==0)
            tx_pool_entries->transaction.reward+=1;  //acabar
          continue;
        }

        if(tx_pool_entries->occupied == 1)
        {
          write_statistics("Invalid block detected", "ERROR");
          sem_post(&(block_ledger->ledger_sem));
          return 1;
        }
        tx_idx[i] = j;
      }
    }
    for(int i = 0;i<config.tx_pool_size;i++)
    {
      (tx_pool_entries+tx_idx[i])->occupied = 1;
    }
    sem_post(&(block_ledger->ledger_sem))
}

int write_statistics(char * message,char * typemsg);