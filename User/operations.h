#pragma once

#include "types.h"
#include "error.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

//deveria de haver uma maneira de se garantir q cada sal é exclusivo né?

int create_bank_account(bank_account_t *acc,uint32_t id, uint32_t balance, char password[]);
int create_bank_ofice();

void *bank_office_process(void *arg);
