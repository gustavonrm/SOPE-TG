#include "operations.h"

int create_bank_account (bank_account_t *acc, uint32_t id, uint32_t balance, char password[]) {
  if (strlen (password) < MIN_PASSWORD_LEN || strlen (password) > MAX_PASSWORD_LEN)
    return ACC_CREATE_ERR;

  char saltedPass[SALT_LEN + strlen (password) + 1];

  if (id == ADMIN_ACCOUNT_ID && balance == 0) {
    acc->account_id = id;
    acc->balance = balance;
  } else {
    if (id < 1 || id >= MAX_BANK_ACCOUNTS ||
      balance < MIN_BALANCE || balance > MAX_BALANCE)
        return ACC_CREATE_ERR;
  }

  acc->account_id = id;
  acc->balance = balance;

  char salt[SALT_LEN + 1];
  gen_salt (salt);
  sprintf (acc->salt, "%s", salt);

  strcpy (saltedPass, acc->salt);
  strcat (saltedPass, password);

  get_hash (saltedPass, acc->hash);

  return 0;
}

ret_code_t transfer_between_accounts (bank_account_t *src, bank_account_t *dest, uint32_t ammount){
  ret_code_t ret = RC_OK;
  uint32_t new_src_balance = src->balance - ammount;
  uint32_t new_dest_balance = dest->balance + ammount;

  ///errors
  if (src->account_id == 0)
    return ret = RC_OP_NALLOW;

  //CHECK ALL ACCOUNTS IF ACC DONEST EXIST RET = ID_NOT_FOUND
  
  if (src->account_id == dest->account_id)
    return ret = RC_SAME_ID;

  if (new_src_balance < MIN_BALANCE)
    return ret = RC_NO_FUNDS;

  if (new_dest_balance > MAX_BALANCE)
    return ret = RC_TOO_HIGH;

  if (dest->account_id < 1 || dest->account_id >= MAX_BANK_ACCOUNTS)
    return ret = RC_OTHER;

  if (ammount < 1 || ammount > MAX_BALANCE)
    return ret = RC_OTHER;

  //proceed opperation
  src->balance -= ammount;
  dest->balance += ammount;

  return ret;
}

int verifyIfAdmin (bank_account_t *admin, uint32_t id, uint32_t balance, char password[]) {
  if (id != ADMIN_ACCOUNT_ID || balance != 0)
    return -1;
  
  char saltedPass[SALT_LEN + MAX_PASSWORD_LEN];

  strcpy (saltedPass, admin ->salt);
  strcat (saltedPass, password);

  char hash[HASH_LEN];

  get_hash (saltedPass, hash);

  return strncmp (admin -> hash, hash, 64);
}