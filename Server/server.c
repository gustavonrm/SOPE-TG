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

#include "utils.h"

int _bank_offices;
char _password[MAX_PASSWORD_LEN];
int fd;
int srvFifo;
bank_account_t admin_account;
bank_account_t user_account[MAX_BANK_ACCOUNTS];
static pthread_t offices[];
////IPC////
sem_t producer, consumer;
sync_mech_op_t smo;
sync_role_t role;

int main (int argc, char *argv[]) {
  //TODO REMOVE LATER ONLY FOR TESTTING IN EARLIER STAGES
  if (unlink(SERVER_FIFO_PATH) != 0)
    exit(UNLINK_ERR);

  if (argc != 3) {
    fprintf (stderr, "USAGE: %s <bank_offices> <password>\n", argv[0]);
    exit (ARG_ERR);
  }

  //slog.txt
  fd = open (SERVER_LOGFILE, O_WRONLY | O_TRUNC | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
  if (fd == -1)
    exit (FILE_OPEN_ERR);

  //IPC init ----- //TODO CHANGE WHEN IN IPC IMPLEMENTATION
  smo = SYNC_OP_SEM_INIT;
  role = SYNC_ROLE_CONSUMER;
  sem_init (&consumer, 0, 0);
  logSyncMechSem (fd, 0, smo, role, 1, 0);
  sem_init (&producer, 0, 0);
  role = SYNC_ROLE_PRODUCER;
  logSyncMechSem (fd, 0, smo, role, 2, 0);

  //pass arguments
  if ((_bank_offices = atoi (argv[1])) > MAX_BANK_OFFICES)
    _bank_offices = MAX_BANK_OFFICES;

  strcpy (_password, argv[2]);

  pthread_t offices[_bank_offices + 1];
  offices[0] = pthread_self ();

  //#1 create admin acc
  create_bank_account (&admin_account, ADMIN_ACCOUNT_ID, 0, _password);
  logAccountCreation (fd, 4, &admin_account);
  
  //#2 create electronic banks
  for (int i = 1; i <= _bank_offices; i++) {
    pthread_create (&offices[i], NULL, bank_office_process, NULL); //TODO thread func
    logBankOfficeOpen (fd, i, offices[i]);
  }

  //#3 create FIFO /tmp/secure_srv
  if (mkfifo (SERVER_FIFO_PATH, 0660) != 0)
    exit(MKFIFO_ERR);

  srvFifo = open (SERVER_FIFO_PATH, O_RDONLY);
  if (srvFifo == -1){
    exit(FIFO_OPEN_ERR);
  }

  //fifo echoing, pauses, logs
  //TODO CHECK FOR no pendent processes
  while (1) {
    tlv_request_t request;
    tlv_reply_t reply;

    int tmpFifo;
    char USER_FIFO_NAME[USER_FIFO_PATH_LEN];
    strcpy (USER_FIFO_NAME,USER_FIFO_PATH_PREFIX);
    sprintf (USER_FIFO_NAME,"%d",request.value.header.pid);
  
    if ((read (srvFifo, &request, sizeof (request))) != 0)
      exit(FIFO_READ_ERR);
  
    //process user fifo name
    tmpFifo = open(USER_FIFO_NAME, O_WRONLY);
    if (tmpFifo != 0)
      exit(FIFO_OPEN_ERR);
    
    //thread do stuff
    if ((write (tmpFifo, &reply, sizeof (reply))) != 0)
      exit(FIFO_WRITE_ERR);
  }

  //program should only stop when all requests have been processed
  //#4 unlink fifo - end server
  if (unlink(SERVER_FIFO_PATH) != 0)
    exit (UNLINK_ERR);

  if (close (fd) != 0)
    exit (FILE_CLOSE_ERR);

  return 0;
}
