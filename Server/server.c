#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#include "constants.h"
#include "error.h"
#include "sope.h"
#include "operations.h"

#include "utils.h"

int _bank_offices;
char _password[MAX_PASSWORD_LEN];
int fd;
int sFifo;
bank_account_t admin_account;
bank_account_t user_account[MAX_BANK_ACCOUNTS];
sem_t producer, consumer;

/////////////////////////////////
void *PrintHello(void *arg)
{
  int *x = (int*)arg;
  printf("thread #%ld!\n", pthread_self());
  x++;
  pthread_exit(0);
}
/////////////////////////////////

int main(int argc, char *argv[])
{

  if (argc != 3)
  {
    fprintf(stderr, "USAGE: %s <bank_offices> <password>\n", argv[0]);
    exit(ARG_ERR);
  }

  //slog.txt
  fd = open(SERVER_LOGFILE, O_WRONLY | O_TRUNC | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
  if (fd == -1)
  {
    exit(FILE_OPEN_ERR);
  }

  //IPC init
  /*
  sem_init(&consumer,0,0);
  logSyncMechSem(fd,0,)
  sem_init(&producer,0,0);
  */

  //pass arguments
  if ((_bank_offices = atoi(argv[1])) > MAX_BANK_OFFICES)
  {
    _bank_offices = MAX_BANK_OFFICES;
  }
  strcpy(_password, argv[2]);

  pthread_t offices[_bank_offices + 1];
  offices[0] = pthread_self();

  //#1 create admin acc
  create_bank_account(&admin_account, ADMIN_ACCOUNT_ID, 0, _password);
  logAccountCreation(fd, 4, &admin_account);
  //#2 create electronic banks
  for (int i = 1; i <= _bank_offices; i++)
  {
    pthread_create(&offices[i], NULL, PrintHello, NULL); //TODO thread func
    logBankOfficeOpen(fd, i, offices[i]);
  }

  //#3 create FIFO /tmp/secure_srv
  if (mkfifo(SERVER_FIFO_PATH, 0660) != 0)
  {
    exit(MKFIFO_ERR);
  }

  if ((sFifo = open(SERVER_FIFO_PATH, O_RDONLY)) == -1)
  {
    exit(FIFO_OPEN_ERR);
  }

  //fifo echoing, pauses, logs
  //#4 unlink fifo

  //program should only stop when all requests have been processed
  if (unlink(SERVER_FIFO_PATH) != 0)
  {
    exit(UNLINK_ERR);
  }

  if(close(fd) !=0)
    exit(FILE_CLOSE_ERR);

  return 0;
}
