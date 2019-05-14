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

void _cleanHouse (int srvFifo, int slogFd);

int main (int argc, char *argv[]) {
  if (argc != 3) {
    fprintf (stderr, "USAGE: %s <bank_offices> <password>\n", argv[0]);
    exit (ARG_ERR);
  }

  int slogFd = open (SERVER_LOGFILE, O_WRONLY | O_TRUNC | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
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
    numOffices = MAX_BANK_OFFICES;

  char adminPass[MAX_PASSWORD_LEN];
  strcpy (adminPass, argv[2]);

  pthread_t offices[numOffices];
  offices[0] = pthread_self ();

  bank_account_t *admin_account;
  admin_account = malloc (sizeof(admin_account));
  create_bank_account (admin_account, ADMIN_ACCOUNT_ID, 0, adminPass);
  logAccountCreation (slogFd, 00000, admin_account);

  int officePipe[numOffices +1][2];
  for (int i = 1; i <= numOffices; i++) {
    pipe (officePipe[i]);
    pthread_create (&offices[i], NULL, bank_office_process, &(officePipe[i])); //TODO thread func
    logBankOfficeOpen (slogFd, i, offices[i]);
  }

  
  if (mkfifo (SERVER_FIFO_PATH, 0660) != 0)
    exit (MKFIFO_ERR);

  int srvFifo = open (SERVER_FIFO_PATH, O_RDONLY);
  if (srvFifo == -1)
    exit (FIFO_OPEN_ERR);

  /*while (1) {
    tlv_request_t request;
    //tlv_reply_t reply;
    int nBytesRead;
    //int i = 0;
    //int tmpFifo;
    //char USER_FIFO_PATH[USER_FIFO_PATH_LEN];
   
    nBytesRead = read (srvFifo, &request, sizeof (request));
    
    if (nBytesRead == -1)
        printf ("failed to receive\n");
    if (nBytesRead == 0)
      continue;

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

    if (sizeof (request)>0) {
      printf ("received message\n");
      printf ("TYPE: %d\n", request.type);
      printf ("PASS: %s\n", request.value.create.password);
      printf ("AMOUNT: %d\n", request.value.create.balance);
      //sprintf(USER_FIFO_PATH, "%s%d", USER_FIFO_PATH_PREFIX, request.value.header.pid);

      //process user fifo name
      if ((tmpFifo = open(USER_FIFO_PATH, O_WRONLY)) != 0)
        exit(FIFO_OPEN_ERR);

      //thread do stuff
      if ((write(tmpFifo, &reply, sizeof(reply))) != 0)
        exit(FIFO_WRITE_ERR);
    }
  }*/

  while (1){
    readRequest (srvFifo);
  }

  _cleanHouse (srvFifo, slogFd);
  free (admin_account);

  return 0;
}

void _cleanHouse (int srvFifo, int slogFd) {
  if (close (srvFifo))
    exit (FIFO_CLOSE_ERR);
  if (unlink (SERVER_FIFO_PATH) != 0)
    exit (UNLINK_ERR);

  if (close (slogFd) != 0)
    exit (FILE_CLOSE_ERR);
}
