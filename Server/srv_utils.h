#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "../Common/constants.h"
#include "../Common/types.h"
#include "../Common/error.h"

#define SEM_NAME_EMPTY  "/sem_empty"
#define SEM_NAME_FULL   "/sem_full"

struct queueEl {
  tlv_request_t request;
  struct queueEl *next;
};
typedef struct queueEl queue_el_t;

typedef struct queue {
  queue_el_t *head;
  queue_el_t *tail;
} queue_t;

typedef enum shutdownFlag {
  SF_OFF,       // server hasnt received shutdown from user
  SF_RD_MODE,   // server has received and secure_srv must be in read only mode
  SF_ON         // shutdown pre operations are done
} shutdownFlag_t;

void gen_salt (char *salt);

void get_hash (char *str, char *hash);

int readFifo (int srvFifo,sem_t full, sem_t empty, pthread_mutex_t mut);

int writeToFifo (tlv_reply_t reply,char *path);

void queuePush (tlv_request_t request);

tlv_request_t queuePop ();

void queueDelete ();

int queueEmpty ();

void delay (tlv_request_t request);

ret_code_t checkLogin (bank_account_t *account, char password[]);

void print_request (tlv_request_t request);

