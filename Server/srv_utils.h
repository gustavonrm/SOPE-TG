#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <pthread.h>
#include <time.h>

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

int readFifo (int srvFifo);

void queuePush (tlv_request_t request);

tlv_request_t queuePop ();

void queueDelete ();

tlv_reply_t makeReply(enum ret_code ret, tlv_request_t request);
