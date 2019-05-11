#include "operations.h"

int create_bank_account(bank_account_t *acc, uint32_t id, uint32_t balance, char password[])
{
  char saltedPass[SALT_LEN + strlen(password) + 1];

  //in case its admin
  if (id == ADMIN_ACCOUNT_ID && balance == 0)
  {
    acc->account_id = id;
    acc->balance = balance;
    gen_salt(acc->salt);

    strcpy(saltedPass, acc->salt);
    strcat(saltedPass, password);

    get_hash(saltedPass, acc->hash);

    return 0;
  }

  //the rest
  if (id < 1 || id >= MAX_BANK_ACCOUNTS)
  {
    return ACC_CREATE_ERR;
  }
  if (balance < MIN_BALANCE || balance > MAX_BALANCE)
  {
    return ACC_CREATE_ERR;
  }
  if (strlen(password) < MIN_PASSWORD_LEN || strlen(password) > MAX_PASSWORD_LEN)
  {
    return ACC_CREATE_ERR;
  }

  acc->account_id = id;
  acc->balance = balance;

  gen_salt(acc->salt);

  strcpy(saltedPass, acc->salt);
  strcat(saltedPass, password);

  get_hash(saltedPass, acc->hash);

  return 0;
}
//////////////////THREADS///////////////////////
void *bank_office_process(void *arg)
{
  int *x = (int *)arg;
  printf("thread #%ld!\n", pthread_self());
  x++;
  pthread_exit(0);
}
