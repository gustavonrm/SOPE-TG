#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../Common/types.h"
#include "../Common/error.h"
#include "../Common/sope.h"

#include "srv_utils.h"

int create_bank_account (bank_account_t *acc,uint32_t id, uint32_t balance, char password[]);

ret_code_t transfer_between_accounts (bank_account_t *src, bank_account_t *dest, uint32_t ammount);

ret_code_t verifyIfAdmin (bank_account_t *admin, uint32_t id, char *password);

