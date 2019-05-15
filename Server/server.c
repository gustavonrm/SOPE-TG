#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <mqueue.h>

#include "../Common/constants.h"
#include "../Common/error.h"
#include "../Common/sope.h"

#include "operations.h"
#include "srv_utils.h"

void _cleanUp(int srvFifo, int slogFd);
void *bank_office_process(void *officePipe);

////////GLOBAL/////////
int slogFd;

mqd_t mq;
struct mq_attr attrp;
//mq_getattr(mq,&attrp);

//atrr.mq_flags = 0;                           // blocking read/write
/*
atrr.mq_maxmsg = MAX_QUEUE_LEN;              // maximum number of messages allowed in queue
attr.mq_msgsize = sizeof(tlv_request_t);     // messages are contents of an int
attr.mq_curmsgs = 0;                         // number of messages currently in queue
*/
sem_t empty, full;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

bank_account_t accounts[MAX_BANK_ACCOUNTS];
int acc_index = 0;

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

  mq = mq_open("/mqueue", O_RDWR | O_CREAT, 0660, NULL); //TODO need to create the struct
  if (mq == -1)
  {
    printf("Error opening mQ\n");
  }

  // IPC init
  // TODO CHANGE WHEN IN IPC IMPLEMENTATION
  sem_init(&empty, 0, numOffices);
  sem_init(&full, 0, 0);
  logSyncMechSem(slogFd, 0, smo, role, numOffices, 0);
  role = SYNC_ROLE_PRODUCER;
  logSyncMechSem(slogFd, 0, smo, role, 0, 0);

  char adminPass[MAX_PASSWORD_LEN];
  strcpy(adminPass, argv[2]);

  bank_account_t *admin_account;
  admin_account = malloc(sizeof(bank_account_t));

  if (create_bank_account(admin_account, ADMIN_ACCOUNT_ID, 0, adminPass) != 0)
    return ACC_CREATE_ERR;
  //accounts[0] = admin_account;
  logAccountCreation(slogFd, 00000, admin_account);

  int officePipe[numOffices + 1][2];
  for (int i = 1; i <= numOffices; i++)
  {
    pipe(officePipe[i]);                                                      //TODO substitute with msgQ
    pthread_create(&offices[i], NULL, bank_office_process, &(officePipe[i])); //TODO thread func
    logBankOfficeOpen(slogFd, i, offices[i]);
  }

  if (mkfifo(SERVER_FIFO_PATH, S_IRWUSR | S_IRWGRP) != 0)
    exit(MKFIFO_ERR);

  int srvFifo = open(SERVER_FIFO_PATH, O_RDONLY);
  if (srvFifo == -1)
    exit(FIFO_OPEN_ERR);

  while (1)
  {
    //IPC should be inside read i think
    sem_wait(&empty);
    pthread_mutex_lock(&mut);

    readFifo(srvFifo,mq); // TODO tem de receber o pipe e comunicar
    
    sem_post(&full);
  }

  _cleanUp(srvFifo, slogFd);
  free(admin_account);

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
}

//////////////////THREADS///////////////////////
void *bank_office_process(void *officePipe)
{
  int *pipe_fd = (int *)officePipe;
  close(pipe_fd[WRITE]);

  printf("thread #%ld!\n", pthread_self());

  while (1)
  {
    tlv_request_t request;
    tlv_reply_t reply;
    int tmpFifo;
    char USER_FIFO_PATH[USER_FIFO_PATH_LEN];

    //1# IPC
    sem_wait(&full);
    pthread_mutex_lock(&mut);

    printf("thread %ld working\n",pthread_self());
    //2#receive
    if (read(pipe_fd[READ], &request, sizeof(request)) == 0)
      printf("pipe received\n");
    if (mq_receive(mq, (char *)&request, sizeof(request), NULL) == 0)
      printf("mq received\n");

    //fifo name
    sprintf(USER_FIFO_PATH, "%s%d", USER_FIFO_PATH_PREFIX, getpid());
    tmpFifo = open(USER_FIFO_PATH, O_WRONLY | O_NONBLOCK);

    switch (request.type)
    {
    case OP_CREATE_ACCOUNT:
      //bank account needs to chek if is admin who asked for it
      if (create_bank_account(&accounts[acc_index++], request.value.create.account_id, request.value.create.balance, request.value.create.password) != 0)
        return (int *)2;
      logAccountCreation(slogFd, request.value.create.account_id, &accounts[acc_index]);
      write(tmpFifo, &reply, sizeof(reply));
      break;
    case OP_BALANCE: //checked on user
      break;
    case OP_TRANSFER:
      break;
    case OP_SHUTDOWN:
      //do smth to end all processes and stuff
      pthread_exit(0);
      break;
    case __OP_MAX_NUMBER:
      break;
    }

    //3# IPC
    pthread_mutex_unlock(&mut);
    sem_post(&empty);
    printf("thread %ld ended\n",pthread_self());
  }
  printf("thread #%ld!\n", pthread_self());
}
