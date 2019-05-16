#include "operations.h"

ret_code_t create_bank_account (bank_account_t *acc, uint32_t id, uint32_t balance, char password[]) {
  if (strlen (password) < MIN_PASSWORD_LEN || strlen (password) > MAX_PASSWORD_LEN)
    return RC_OTHER;

  char saltedPass[SALT_LEN + strlen (password) + 1];

  if (id == ADMIN_ACCOUNT_ID && balance == 0) {
    acc->account_id = id;
    acc->balance = balance;
  } else {
    if (id < 1 || id >= MAX_BANK_ACCOUNTS ||
      balance < MIN_BALANCE || balance > MAX_BALANCE)
        return RC_OTHER;
  }

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

  if (src->account_id == 0)
    return RC_OP_NALLOW;
  
  if (src->account_id == dest->account_id)
    return RC_SAME_ID;

  if (new_src_balance < MIN_BALANCE)
    return RC_NO_FUNDS;

  if (new_dest_balance > MAX_BALANCE)
    return RC_TOO_HIGH;

  if (dest->account_id < 1 || dest->account_id >= MAX_BANK_ACCOUNTS)
    return RC_OTHER;

  if (ammount < 1 || ammount > MAX_BALANCE)
    return RC_OTHER;

  //proceed opperation
  src->balance = new_src_balance;
  dest->balance = new_dest_balance;

  return RC_OK;
}