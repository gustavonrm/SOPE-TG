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

struct queueEl {
  tlv_request_t request;
  struct queueEl *next;
};
typedef struct queueEl queue_el_t;

typedef struct queue {
  queue_el_t *head;
  queue_el_t *tail;
} queue_t;

void gen_salt (char *salt);

void get_hash (char *str, char *hash);

//int readFifo (int srvFifo);
int readFifo (int srvFifo,sem_t full, sem_t empty, pthread_mutex_t mut);

int writeToFifo (tlv_reply_t reply,char *path);

void queuePush (tlv_request_t request);

tlv_request_t queuePop ();

void queueDelete ();

ret_code_t checkLogin(bank_account_t *account, uint32_t id, char password[]);

void print_request(tlv_request_t request);

