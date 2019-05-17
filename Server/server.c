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
#include "../Common/reply.h"

#include "operations.h"
#include "srv_utils.h"

void _cleanUp(int srvFifo, int slogFd);
void *bank_office_process(void *arg);

////////GLOBAL/////////
int slogFd;

sem_t empty, full;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

bank_account_t accounts[MAX_BANK_ACCOUNTS] = {};
bank_account_t admin_account;

int whileLoop = -1;

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    fprintf(stderr, "USAGE: %s <bank_offices> <password>\n", argv[0]);
    exit(ARG_ERR);
  }

  slogFd = open(SERVER_LOGFILE, O_WRONLY | O_TRUNC | O_CREAT, S_IRWUSR | S_IRGRP | S_IROTH);
  if (slogFd == -1)
    exit(FILE_OPEN_ERR);

  int numOffices = atoi(argv[1]);
  if (numOffices > MAX_BANK_OFFICES)
    return INVALID_INPUT_ERR;

  pthread_t offices[numOffices];
  offices[0] = pthread_self();

  // IPC init
  // TODO CHANGE WHEN IN IPC IMPLEMENTATION
  logSyncMechSem(slogFd, 0, SYNC_OP_SEM_INIT, SYNC_ROLE_PRODUCER, getpid(), 0);
  sem_init(&full, 0, 0);
  logSyncMechSem(slogFd, 0, SYNC_OP_SEM_INIT, SYNC_ROLE_PRODUCER, getpid(), numOffices);
  sem_init(&empty, 0, numOffices);

  if (create_bank_account(&admin_account, ADMIN_ACCOUNT_ID, 0, argv[2]) != 0)
    return ACC_CREATE_ERR;
  accounts[0] = admin_account;
  logAccountCreation(slogFd, 0000, &admin_account);

  for (int i = 1; i <= numOffices; i++)
  {
    pthread_create(&offices[i], NULL, bank_office_process, (void *)&i);
    logBankOfficeOpen(slogFd, i, offices[i]);
  }

  if (mkfifo(SERVER_FIFO_PATH, S_IRWUSR | S_IRWGRP) != 0)
    exit(MKFIFO_ERR);

  int srvFifo = open(SERVER_FIFO_PATH, O_RDONLY);
  if (srvFifo == -1)
    exit(FIFO_OPEN_ERR);

  int nBytes;
  int sem_val;
  while (whileLoop  == -1) {
    tlv_request_t request;

    nBytes = read(srvFifo, &request, sizeof(op_type_t) + sizeof(uint32_t));
    if (nBytes == -1)
      exit(FIFO_READ_ERR);

    if (nBytes == 0)
      continue;

    nBytes = read(srvFifo, &request.value, request.length);
    if (nBytes == -1)
      exit(FIFO_READ_ERR);

    if (nBytes == 0)
      continue;

    print_request(request);
    //log
    
    sem_getvalue(&empty, &sem_val);
    logSyncMechSem(slogFd, 0, SYNC_OP_COND_WAIT, SYNC_ROLE_PRODUCER, getpid(), sem_val);
    //ipc
    sem_wait(&empty);
    //log
    logSyncMech(slogFd, 0, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_PRODUCER, getpid());
    //ipc
    pthread_mutex_lock(&mut);
    //critical region
    queuePush(request);
    //ipc
    pthread_mutex_unlock(&mut);
    //log
    logSyncMech(slogFd, 0, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_PRODUCER, getpid());
    //ipc
    sem_post(&full);
    //log
    sem_getvalue(&full, &sem_val);
    logSyncMechSem(slogFd, 0, SYNC_OP_SEM_POST, SYNC_ROLE_PRODUCER, getpid(), sem_val);
  }
  printf ("Break free!\n");
  fchmod (srvFifo, S_IRUSR | S_IRGRP | S_IROTH);
  
  sem_getvalue (&empty, &sem_val);
  while (sem_val != numOffices)
    sem_getvalue (&empty, &sem_val);

  _cleanUp(srvFifo, slogFd);

  return 0;
}

void _cleanUp(int srvFifo, int slogFd)
{
  if (close(srvFifo))
    exit(FIFO_CLOSE_ERR);

  if (unlink(SERVER_FIFO_PATH) != 0)
    exit(UNLINK_ERR);

  if (close(slogFd) != 0)
    exit(FILE_CLOSE_ERR);

  queueDelete();
}

//////////////////THREADS///////////////////////
void *bank_office_process(void *arg)
{
  printf("thread #%ld!\n", pthread_self());
  int index = (*(int *)arg);
  while (1)
  {
    tlv_request_t request;
    tlv_reply_t reply;
    char USER_FIFO_PATH[USER_FIFO_PATH_LEN];
    int sem_val;

    //log
    sem_getvalue(&full, &sem_val);
    logSyncMechSem(slogFd, index, SYNC_OP_SEM_WAIT, SYNC_ROLE_CONSUMER, getpid(), sem_val);
    //1# IPC
    sem_wait(&full);
    //log
    logSyncMech(slogFd, index, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_PRODUCER, getpid());
    //ipc
    pthread_mutex_lock(&mut);
    //2#receive -- CRITICAL REGION
    request = queuePop();
    //apos acessar conta!!!! atraso
    delay(request);
    logDelay(slogFd, index, request.value.header.op_delay_ms); //logSyncDelay()???
    //fifo name
    sprintf(USER_FIFO_PATH, "%s%d", USER_FIFO_PATH_PREFIX, request.value.header.pid);

    switch (request.type)
    {
    case OP_CREATE_ACCOUNT:
    {
      ret_code_t ret;
      uint32_t id = request.value.create.account_id;

      ret = checkLogin(&(accounts[0]), request.value.header.password);
      if (ret != RC_OK)
      {
        reply = makeErrorReply(&request, RC_LOGIN_FAIL);
        writeToFifo(reply, USER_FIFO_PATH);
        break;
      }

      if (request.value.header.account_id != ADMIN_ACCOUNT_ID)
      {
        reply = makeErrorReply(&request, RC_OP_NALLOW);
        writeToFifo(reply, USER_FIFO_PATH);
        break;
      }

      if (accounts[id].account_id == id)
      {
        reply = makeErrorReply(&request, RC_ID_IN_USE);
        writeToFifo(reply, USER_FIFO_PATH);
        break;
      }

      ret = create_bank_account(&accounts[id], request.value.create.account_id, request.value.create.balance, request.value.create.password);
      if (ret == RC_OTHER)
      {
        reply = makeErrorReply(&request, RC_OTHER);
        writeToFifo(reply, USER_FIFO_PATH);
        break;
      }
      reply = makeReply(&request, accounts[id].balance);

      logAccountCreation(slogFd, request.value.create.account_id, &accounts[id]);
      writeToFifo(reply, USER_FIFO_PATH);
      break;
    }

    case OP_BALANCE:
    {
      ret_code_t ret;
      uint32_t id = request.value.header.account_id;

      if(accounts[id].account_id == 0){
        reply = makeErrorReply(&request, RC_ID_NOT_FOUND);
        writeToFifo(reply, USER_FIFO_PATH);
        break;
      }

      ret = checkLogin(&(accounts[id]), request.value.header.password);
      if (ret != RC_OK)
      {
        reply = makeErrorReply(&request, ret);
        writeToFifo(reply, USER_FIFO_PATH);
        break;
      }
      if (request.value.header.account_id == 0)
      {
        reply = makeErrorReply(&request, RC_OP_NALLOW);
        writeToFifo(reply, USER_FIFO_PATH);
        break;
      }
      reply = makeReply(&request, accounts[id].balance);
      writeToFifo(reply, USER_FIFO_PATH);
      break;
    }

    case OP_TRANSFER:
    {
      ret_code_t ret;
      uint32_t src_id = request.value.header.account_id;
      uint32_t dest_id = request.value.transfer.account_id;

      if (request.value.header.account_id == ADMIN_ACCOUNT_ID)
      {
        reply = makeErrorReply(&request, RC_OP_NALLOW);
        writeToFifo(reply, USER_FIFO_PATH);
        break;
      }

      if(accounts[src_id].account_id == 0){
        reply = makeErrorReply(&request, RC_ID_NOT_FOUND);
        reply.value.transfer.balance=0;
        writeToFifo(reply, USER_FIFO_PATH);
        break;
      }

      ret = checkLogin(&(accounts[src_id]), request.value.header.password);
      if (ret != RC_OK)
      {
        reply = makeErrorReply(&request, RC_LOGIN_FAIL);
        writeToFifo(reply, USER_FIFO_PATH);
        break;
      }

      ret = transfer_between_accounts(&accounts[src_id],&accounts[dest_id],request.value.transfer.amount);
      if(ret != RC_OK)
      {
        reply = makeErrorReply(&request, ret);
        writeToFifo(reply, USER_FIFO_PATH);
        break;
      }
      
      reply = makeReply(&request, accounts[src_id].balance);
      writeToFifo(reply, USER_FIFO_PATH);
      break;
    }

    case OP_SHUTDOWN:
    {
      ret_code_t ret;

      ret = checkLogin (&admin_account, request.value.header.password);
      if (ret != RC_OK) {
        reply = makeErrorReply (&request, ret);
        writeToFifo (reply, USER_FIFO_PATH);
        break;
      }

      if (request.value.header.account_id != ADMIN_ACCOUNT_ID) {
        reply = makeErrorReply (&request, RC_OP_NALLOW);
        writeToFifo (reply, USER_FIFO_PATH);
        break;
      }
        
      sem_getvalue (&empty, &whileLoop);
      reply = makeReply (&request, (uint32_t)whileLoop);
      writeToFifo (reply, USER_FIFO_PATH);

      break;
    }
    case __OP_MAX_NUMBER:
      break;
    }

    //3# IPC
    pthread_mutex_unlock(&mut);
    //log
    logSyncMech(slogFd, index, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_CONSUMER, getpid());
    //IPC
    sem_post(&empty);
    //log
    sem_getvalue(&empty, &sem_val);
    logSyncMechSem(slogFd, index, SYNC_OP_SEM_POST, SYNC_ROLE_CONSUMER, getpid(), sem_val);
  }

  printf("thread #%ld!\n", pthread_self());
}
