#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <pthread.h>
#include <mqueue.h>

#include "../Common/constants.h"
#include "../Common/types.h"
#include "../Common/error.h"

void gen_salt (char *salt);

void get_hash (char *str, char *hash);

int readFifo (int srvFifo,mqd_t mq);


