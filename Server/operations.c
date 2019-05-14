#include "operations.h"

int create_bank_account (bank_account_t *acc, uint32_t id, uint32_t balance, char password[]) {
  if (strlen(password) < MIN_PASSWORD_LEN || strlen(password) > MAX_PASSWORD_LEN)
    return ACC_CREATE_ERR;

  char saltedPass[SALT_LEN + strlen(password) + 1];

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

  char salt[SALT_LEN +1];
  gen_salt (salt);
  sprintf (acc -> salt, "%s", salt);

  strcpy (saltedPass, acc->salt);
  strcat (saltedPass, password);

  get_hash (saltedPass, acc->hash);
  
  return 0;
}
//////////////////THREADS///////////////////////
void *bank_office_process (void *arg) {
  int *x = (int *)arg;
  printf ("thread #%ld!\n", pthread_self ());
  x++;
  pthread_exit (0);
}
