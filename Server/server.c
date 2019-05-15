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

void _cleanUp(int srvFifo, int slogFd);
void *bank_office_process(void *arg);

////////GLOBAL/////////
int slogFd;

sem_t empty, full;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

bank_account_t accounts[MAX_BANK_ACCOUNTS];
bank_account_t admin_account;
int acc_index = 0;

queue_t requests;

//logs
sync_mech_op_t smo = SYNC_OP_SEM_INIT;
sync_role_t role = SYNC_ROLE_CONSUMER;

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
  sem_init(&empty, 0, numOffices);
  sem_init(&full, 0, 0);
  logSyncMechSem(slogFd, 0, smo, role, numOffices, 0);
  role = SYNC_ROLE_PRODUCER;
  logSyncMechSem(slogFd, 0, smo, role, 0, 0);

  if (create_bank_account(&admin_account, ADMIN_ACCOUNT_ID, 0, argv[2]) != 0)
    return ACC_CREATE_ERR;
  accounts[0] = admin_account;
  logAccountCreation(slogFd, 0000, &admin_account);

  for (int i = 1; i <= numOffices; i++)
  {
    pthread_create(&offices[i], NULL, bank_office_process, NULL); //TODO thread func
    logBankOfficeOpen(slogFd, i, offices[i]);
  }

  if (mkfifo(SERVER_FIFO_PATH, S_IRWUSR | S_IRWGRP) != 0)
    exit(MKFIFO_ERR);

  int srvFifo = open(SERVER_FIFO_PATH, O_RDONLY);
  if (srvFifo == -1)
    exit(FIFO_OPEN_ERR);

  while (1)
  {

    int nBytes = 0;

    while (1)
    {
      tlv_request_t request;

      nBytes = read(srvFifo, &request, sizeof(op_type_t) + sizeof(uint32_t));
      if (nBytes == -1)
        exit(FIFO_READ_ERR);

      if (nBytes == 0)
        break;

      nBytes = read(srvFifo, &request.value, request.length);
      if (nBytes == -1)
        exit(FIFO_READ_ERR);

      if (nBytes == 0)
        break;

      printf("li coisas\n");

      sem_wait(&empty);
      pthread_mutex_lock(&mut);
      printf("producer\n");
      queuePush(request);
      pthread_mutex_unlock(&mut);
      sem_post(&full);
    }
  }

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
  //ele implica com o arg por causa do -Werror
  int *i = (int *)arg;
  i++;
  while (1)
  {
    tlv_request_t request;
    tlv_reply_t reply;
    int tmpFifo;
    char USER_FIFO_PATH[USER_FIFO_PATH_LEN];

    printf("estou a funcionar\n");
    //1# IPC
    sem_wait(&full);
    pthread_mutex_lock(&mut);
    printf("consumer\n");
    //2#receive
    request = queuePop();
    //print_request(request);
    //fifo name
    sprintf(USER_FIFO_PATH, "%s%d", USER_FIFO_PATH_PREFIX, request.value.header.pid);
    tmpFifo = open(USER_FIFO_PATH, O_WRONLY | O_NONBLOCK);

    switch (request.type)
    {
    case OP_CREATE_ACCOUNT:
     /* if (verifyIfAdmin(&admin_account, request.value.create.account_id, request.value.create.balance, request.value.create.password) != 0)
        // Retorna para o usr pelo fifo o tlv reply a dizer OP_NALLOW
        if (create_bank_account(&accounts[acc_index++], request.value.create.account_id, request.value.create.balance, request.value.create.password) != 0)
          return (int *)2; //?????*/
      printf("create acc\n");
      logAccountCreation(slogFd, request.value.create.account_id, &accounts[acc_index]);
      write(tmpFifo, &reply, sizeof(reply));
      break;

    case OP_BALANCE: //checked on user
      if(request.value.header.account_id != 0){ //check if is not admin 
          reply.type= request.type;
          reply.length = request.length;

          reply.value.header.account_id=request.value.header.account_id;
          reply.value.header.ret_code=0;
          reply.value.balance.balance=accounts[(int)request.value.header.account_id].balance;
      }
      //print_request(request);
      write(tmpFifo,&reply,sizeof(reply));
      break;

    case OP_TRANSFER:
      break;

    case OP_SHUTDOWN:
      if (verifyIfAdmin(&admin_account, request.value.create.account_id, request.value.create.balance, request.value.create.password) != 0)
        // Retorna para o usr pelo fifo o tlv reply a dizer OP_NALLOW
        pthread_exit(0);
      break;

    case __OP_MAX_NUMBER:
      break;
    }

    //3# IPC
    pthread_mutex_unlock(&mut);
    sem_post(&empty);
  }

  printf("thread #%ld!\n", pthread_self());
}
