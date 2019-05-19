#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "error.h"
#include "sope.h"

#include "srv_utils.h"

ret_code_t create_bank_account (bank_account_t *acc, uint32_t id, uint32_t balance, char password[]);

ret_code_t transfer_between_accounts (bank_account_t *src, bank_account_t *dest, uint32_t ammount);
