#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#include "../Common/constants.h"
#include "../Common/error.h"
#include "../Common/sope.h"

#include "operations.h"
#include "srv_utils.h"

void _cleanUp (int srvFifo, int slogFd);

int main (int argc, char *argv[]) {
  
  if (argc != 3) {
    fprintf (stderr, "USAGE: %s <bank_offices> <password>\n", argv[0]);
    exit (ARG_ERR);
  }

  int slogFd = open (SERVER_LOGFILE, O_WRONLY | O_TRUNC | O_CREAT, S_IRWUSR | S_IRGRP | S_IROTH);
  if (slogFd == -1)
    exit (FILE_OPEN_ERR);

  // IPC init
  // TODO CHANGE WHEN IN IPC IMPLEMENTATION
  sem_t producer, consumer;
  sync_mech_op_t smo = SYNC_OP_SEM_INIT;
  sync_role_t role = SYNC_ROLE_CONSUMER;
  sem_init (&consumer, 0, 0);
  logSyncMechSem (slogFd, 0, smo, role, 1, 0);
  sem_init (&producer, 0, 0);
  role = SYNC_ROLE_PRODUCER;
  logSyncMechSem (slogFd, 0, smo, role, 2, 0);

  int numOffices = atoi (argv[1]);
  if (numOffices > MAX_BANK_OFFICES)
    return INVALID_INPUT_ERR;

  pthread_t offices[numOffices];
  offices[0] = pthread_self ();

  char adminPass[MAX_PASSWORD_LEN];
  strcpy (adminPass, argv[2]);

  bank_account_t *admin_account;
  admin_account = malloc (sizeof(bank_account_t));
  char adminPass[MAX_PASSWORD_LEN];
  strcpy(adminPass, argv[2]);
  
  if(create_bank_account (admin_account, ADMIN_ACCOUNT_ID, 0, adminPass) != 0)
    return ACC_CREATE_ERR;
  logAccountCreation (slogFd, 00000, admin_account);

  int officePipe[numOffices +1][2];
  for (int i = 1; i <= numOffices; i++) {
    pipe (officePipe[i]);
    pthread_create (&offices[i], NULL, bank_office_process, &(officePipe[i])); //TODO thread func
    logBankOfficeOpen (slogFd, i, offices[i]);
  }
  
  if (mkfifo (SERVER_FIFO_PATH, S_IRWUSR|S_IRWGRP) != 0)
    exit (MKFIFO_ERR);

  int srvFifo = open (SERVER_FIFO_PATH, O_RDONLY);
  if (srvFifo == -1)
    exit (FIFO_OPEN_ERR);

  /*
    switch (request.type) {
    case OP_CREATE_ACCOUNT:
      if (create_bank_account (&user_account[i], request.value.create.account_id, request.value.create.balance, request.value.create.password) != 0)
        return 2;
      logAccountCreation (slogFd, request.value.create.account_id, &user_account[i]);
      i++;
      break;

    case OP_BALANCE:
      break;

    case OP_TRANSFER:
      break;

    case OP_SHUTDOWN:
      break;

    case __OP_MAX_NUMBER:
      break;
    }
  */
  while (1) {
    readRequest (srvFifo);
  }

  _cleanUp (srvFifo, slogFd);
  free (admin_account);

  return 0;
}

void _cleanUp (int srvFifo, int slogFd) {
  if (close (srvFifo))
    exit (FIFO_CLOSE_ERR);
  if (unlink (SERVER_FIFO_PATH) != 0)
    exit (UNLINK_ERR);

  if (close (slogFd) != 0)
    exit (FILE_CLOSE_ERR);
}
