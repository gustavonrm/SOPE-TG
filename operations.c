#include "operations.h"

ret_code_t create_bank_account (bank_account_t *acc, uint32_t id, uint32_t balance, char password[]) {
  char saltedPass[SALT_LEN + strlen (password) + 1];

  acc->account_id = id;
  acc->balance = balance;

  char salt[SALT_LEN + 1];
  gen_salt (salt);
  sprintf (acc->salt, "%s", salt);

  strcpy (saltedPass, acc->salt);
  strcat (saltedPass, password);
  
  get_hash (saltedPass, acc->hash);
  
  return RC_OK;
}

ret_code_t transfer_between_accounts (bank_account_t *src, bank_account_t *dest, uint32_t ammount){
  uint32_t new_src_balance = src->balance - ammount;
  uint32_t new_dest_balance = dest->balance + ammount;

  if ((int)new_src_balance < (int)MIN_BALANCE)
    return RC_NO_FUNDS;

  if ((int)new_dest_balance > (int)MAX_BALANCE)
    return RC_TOO_HIGH;

  src->balance = new_src_balance;
  dest->balance = new_dest_balance;

  return RC_OK;
}