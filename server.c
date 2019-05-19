#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

#include "constants.h"
#include "error.h"
#include "sope.h"
#include "reply.h"

#include "operations.h"
#include "srv_utils.h"

void _cleanUp(int srvFifo, int slogFd, sem_t *empty, sem_t *full);
void *bank_office_process(void *arg);

////////GLOBAL/////////
int slogFd;

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

bank_account_t accounts[MAX_BANK_ACCOUNTS] = {};
bank_account_t admin_account;

shutdownFlag_t shutdownFlag = SF_OFF;
int numOffices;

int main (int argc, char *argv[]) {
  if (argc != 3) {
    fprintf (stderr, "USAGE: %s <bank_offices> <password>\n", argv[0]);
    exit (ARG_ERR);
  }

  slogFd = open (SERVER_LOGFILE, O_WRONLY | O_TRUNC | O_CREAT, S_IRWUSR | S_IRGRP | S_IROTH);
  if (slogFd == -1) {
    perror (strerror (errno));
    exit (FILE_OPEN_ERR);
  }

  numOffices = atoi (argv[1]);
  if (numOffices > MAX_BANK_OFFICES)
    exit (INVALID_INPUT_ERR);

  pthread_t offices[numOffices];
  offices[0] = pthread_self ();

  sem_t *full, *empty;

  logSyncMechSem (slogFd, 0, SYNC_OP_SEM_INIT, SYNC_ROLE_PRODUCER, 0, 0);
  full = sem_open (SEM_NAME_FULL, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, 0);

  logSyncMechSem (slogFd, 0, SYNC_OP_SEM_INIT, SYNC_ROLE_PRODUCER, 0, numOffices);
  empty = sem_open (SEM_NAME_EMPTY, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, numOffices);

  if (create_bank_account (&admin_account, ADMIN_ACCOUNT_ID, 0, argv[2]) != 0)
    return ACC_CREATE_ERR;

  accounts[0] = admin_account;
  logAccountCreation (slogFd, 0000, &admin_account);

  for (int i = 1; i <= numOffices; i++) {
    pthread_create (&offices[i], NULL, bank_office_process, (void *)&i);
    logBankOfficeOpen (slogFd, i, offices[i]);
  }

  if (mkfifo (SERVER_FIFO_PATH, S_IRWUSR | S_IRWGRP) != 0)
    exit (MKFIFO_ERR);

  int srvFifo = open (SERVER_FIFO_PATH, O_RDONLY);
  if (srvFifo == -1)
    exit (FIFO_OPEN_ERR);

  int nBytes;
  int sem_val;
  

  while (1) {
    if (shutdownFlag == SF_RD_MODE) {
      fchmod (srvFifo, S_IRUSR | S_IRGRP | S_IROTH);
      shutdownFlag = SF_ON;
    }
    
    tlv_request_t request;

    nBytes = read (srvFifo, &request, sizeof (op_type_t) + sizeof (uint32_t));
    if (nBytes == -1)
      exit (FIFO_READ_ERR);

    if (nBytes == 0 && shutdownFlag == SF_ON)
      break;

    if (nBytes == 0)
      continue;

    nBytes = read (srvFifo, &request.value, request.length);
    if (nBytes == -1)
      exit (FIFO_READ_ERR);

    if (nBytes == 0 && shutdownFlag == SF_ON)
      break;

    if (nBytes == 0)
      continue;

    sem_getvalue (empty, &sem_val);
    logSyncMechSem (slogFd, 0, SYNC_OP_COND_WAIT, SYNC_ROLE_PRODUCER, request.value.header.pid, sem_val); 

    sem_wait (empty);

    logSyncMech (slogFd, 0, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_PRODUCER, request.value.header.pid); 
    pthread_mutex_lock (&mut);

    queuePush (request);

    pthread_mutex_unlock (&mut);
    logSyncMech (slogFd, 0, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_PRODUCER, request.value.header.pid); 

    sem_post (full);

    sem_getvalue (full, &sem_val);
    logSyncMechSem (slogFd, 0, SYNC_OP_SEM_POST, SYNC_ROLE_PRODUCER, request.value.header.pid, sem_val); 
  }

  sem_getvalue (empty, &sem_val);

  while (sem_val != numOffices)
    sem_getvalue (empty, &sem_val);
  
  for (int i = 1; i < numOffices; i++)
    pthread_join (offices[i], NULL);

  _cleanUp (srvFifo, slogFd, empty, full);

  return 0;
}

void _cleanUp (int srvFifo, int slogFd, sem_t *empty, sem_t *full) {
  if (close (srvFifo))
    perror (strerror (errno));

  if (unlink (SERVER_FIFO_PATH) != 0)
    perror (strerror (errno));

  if (close (slogFd) != 0)
    perror (strerror (errno));

  if (sem_close (empty) != 0)
    perror (strerror (errno));

  if (sem_close (full) != 0)
    perror (strerror (errno));

  if (sem_unlink (SEM_NAME_EMPTY) != 0)
    perror (strerror (errno));

  if (sem_unlink (SEM_NAME_FULL) != 0)
    perror (strerror (errno));

  queueDelete ();
}

//////////////////THREADS///////////////////////
void *bank_office_process (void *arg) {
  
  int index = (*(int *)arg);
  pthread_detach (pthread_self ());

  while (1) {    
    tlv_request_t request;
    tlv_reply_t reply;
    char USER_FIFO_PATH[USER_FIFO_PATH_LEN];
    int sem_val;
    sem_t *full, *empty;

    empty = sem_open (SEM_NAME_EMPTY, O_RDWR);
    full = sem_open (SEM_NAME_FULL, O_RDWR);

    sem_getvalue (full, &sem_val);
    logSyncMechSem (slogFd, index, SYNC_OP_SEM_WAIT, SYNC_ROLE_CONSUMER,  0, sem_val);//0

    sem_wait (full);

    logSyncMech (slogFd, index, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_PRODUCER, 0); //0
    pthread_mutex_lock (&mut);

    if (shutdownFlag == SF_ON && queueEmpty ())
      pthread_exit (NULL);

    request = queuePop ();

    delay (request);
    if(request.type == OP_SHUTDOWN){
      logDelay (slogFd, index,request.value.header.op_delay_ms); 
    }else{
      logSyncDelay (slogFd, index,request.value.header.account_id,request.value.header.op_delay_ms); 
    }

    sprintf (USER_FIFO_PATH, "%s%d", USER_FIFO_PATH_PREFIX, request.value.header.pid);

    switch (request.type) {
    case OP_CREATE_ACCOUNT:
    {
      ret_code_t ret;
      uint32_t id = request.value.create.account_id;

      ret = checkLogin (&(accounts[request.value.header.account_id]), request.value.header.password);
      if (ret != RC_OK) {
        reply = makeErrorReply (&request, ret);
        srv_writeToFifo (reply, USER_FIFO_PATH);
        break;
      }

      if (request.value.header.account_id != ADMIN_ACCOUNT_ID) {
        reply = makeErrorReply (&request, RC_OP_NALLOW);
        srv_writeToFifo (reply, USER_FIFO_PATH);
        break;
      }

      if (accounts[id].account_id == id) {
        reply = makeErrorReply (&request, RC_ID_IN_USE);
        srv_writeToFifo (reply, USER_FIFO_PATH);
        break;
      }

      ret = create_bank_account (&accounts[id], request.value.create.account_id, request.value.create.balance, request.value.create.password);
      if (ret != RC_OK) {
        reply = makeErrorReply (&request, ret);
        srv_writeToFifo (reply, USER_FIFO_PATH);
        break;
      }

      logAccountCreation (slogFd, request.value.create.account_id, &accounts[id]);
      
      reply = makeReply (&request, accounts[id].balance);
      srv_writeToFifo (reply, USER_FIFO_PATH);
      
      break;
    }

    case OP_BALANCE:
    {
      ret_code_t ret;
      uint32_t id = request.value.header.account_id;

      ret = checkLogin (&(accounts[id]), request.value.header.password);
      if (ret != RC_OK) {
        reply = makeErrorReply (&request, ret);
        srv_writeToFifo (reply, USER_FIFO_PATH);
        break;
      }

      if (request.value.header.account_id == 0) {
        reply = makeErrorReply (&request, RC_OP_NALLOW);
        srv_writeToFifo (reply, USER_FIFO_PATH);
        break;
      }

      if (accounts[id].account_id == 0) {
        reply = makeErrorReply (&request, RC_ID_NOT_FOUND);
        srv_writeToFifo (reply, USER_FIFO_PATH);
        break;
      }

      reply = makeReply (&request, accounts[id].balance);
      srv_writeToFifo (reply, USER_FIFO_PATH);
      break;
    }

    case OP_TRANSFER:
    {
      ret_code_t ret;
      uint32_t src_id = request.value.header.account_id;
      uint32_t dest_id = request.value.transfer.account_id;

      ret = checkLogin (&(accounts[src_id]), request.value.header.password);
      if (ret != RC_OK) {
        reply = makeErrorReply (&request, ret);
        reply.value.transfer.balance = accounts[request.value.header.account_id].balance;
        srv_writeToFifo (reply, USER_FIFO_PATH);
        break;
      }

      if (request.value.header.account_id == ADMIN_ACCOUNT_ID) {
        reply = makeErrorReply (&request, RC_OP_NALLOW);
        reply.value.transfer.balance = accounts[request.value.header.account_id].balance;
        srv_writeToFifo (reply, USER_FIFO_PATH);
        break;
      }

      if (accounts[src_id].account_id != src_id || accounts[dest_id].account_id != dest_id) {
        reply = makeErrorReply (&request, RC_ID_NOT_FOUND);
        reply.value.transfer.balance = accounts[request.value.header.account_id].balance;
        srv_writeToFifo(reply, USER_FIFO_PATH);
        break;
      }

      if (src_id == dest_id) {
        reply = makeErrorReply (&request, RC_SAME_ID);
        reply.value.transfer.balance = accounts[request.value.header.account_id].balance;
        srv_writeToFifo (reply, USER_FIFO_PATH);
        break;
      }
      
      ret = transfer_between_accounts(&accounts[src_id], &accounts[dest_id], request.value.transfer.amount);
      if (ret != RC_OK) {
        reply = makeErrorReply(&request, ret);
        reply.value.transfer.balance = accounts[request.value.header.account_id].balance;
        srv_writeToFifo(reply, USER_FIFO_PATH);
        break;
      }

      reply = makeReply (&request, accounts[src_id].balance);
      srv_writeToFifo (reply, USER_FIFO_PATH);
      break;
    }

    case OP_SHUTDOWN:
    {
      ret_code_t ret;

      ret = checkLogin (&(accounts[request.value.header.account_id]), request.value.header.password);
      if (ret != RC_OK) {
        reply = makeErrorReply (&request, ret);
        srv_writeToFifo (reply, USER_FIFO_PATH);
        break;
      }

      if (request.value.header.account_id != ADMIN_ACCOUNT_ID) {
        reply = makeErrorReply (&request, RC_OP_NALLOW);
        srv_writeToFifo (reply, USER_FIFO_PATH);
        break;
      }
      
      int sem_val;

      sem_getvalue (empty, &sem_val);
      reply = makeReply (&request,  numOffices - (uint32_t)sem_val);
      srv_writeToFifo (reply, USER_FIFO_PATH);
      shutdownFlag = SF_RD_MODE;

      break;
    }

    case __OP_MAX_NUMBER:
      break;
    }

    pthread_mutex_unlock (&mut);
    logSyncMech(slogFd, index, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_CONSUMER, request.value.header.pid);
    
    sem_post (empty);
    
    sem_getvalue (empty, &sem_val);
    logSyncMechSem (slogFd, index, SYNC_OP_SEM_POST, SYNC_ROLE_CONSUMER, request.value.header.pid, sem_val);
  }
}
