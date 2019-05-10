#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include "constants.h"
#include "error.h"
#include "sope.h"
#include "operations.h"

#include "utils.h"

int  _bank_offices;
char  _password[MAX_PASSWORD_LEN];
int  fd;
int  sFifo;
bank_account_t admin_account;

int main (int argc, char *argv[]) {
  if (argc != 3) {
    fprintf (stderr, "USAGE: %s <bank_offices> <password>\n", argv[0]);
    exit (ARG_ERR);
  }
  
  //slog.txt
  fd = open (SERVER_LOGFILE, O_WRONLY|O_TRUNC|O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
  if ( fd == -1) {
    exit (FILE_OPEN_ERR);
  }
  
  //pass arguments
  if ((_bank_offices = atoi (argv[1])) > MAX_BANK_OFFICES) {
    _bank_offices = MAX_BANK_OFFICES;
  }
  strcpy(_password, argv[2]);
  
  //#1 create admin acc
  create_bank_account (&admin_account, ADMIN_ACCOUNT_ID, 0, _password);
  logAccountCreation (fd, 0, &admin_account);

  //#2 create electronic banks
  for (int i=0; i < _bank_offices; i++) {
    logBankOfficeOpen (fd, 0, pthread_self ());
  } 

  //#3 create FIFO /tmp/secure_srv
  if (mkfifo (SERVER_FIFO_PATH, 0660) != 0) {
    exit (MKFIFO_ERR);
  }

  if ((sFifo = open (SERVER_FIFO_PATH, O_RDONLY)) == -1) {
    exit (FIFO_OPEN_ERR);
  }

  //fifo echoing, pauses, logs
  //#4 unlink fifo

  //program should only stop when all requests have been processed 
  if (unlink (SERVER_FIFO_PATH) != 0){
    exit (UNLINK_ERR);
  }

  return 0;
}
