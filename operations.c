#include "operations.h"

//n sei se é suposto fazer ou usar o q é dado, mas da a sensacao que as logs e so pa comunicaçÃO
int create_bank_account(bank_account_t *acc, uint32_t id, uint32_t balance, char password[])
{
    //in case its admin
    if (id == ADMIN_ACCOUNT_ID && balance == 0)
    {
        acc->account_id = id;
        acc->balance = balance;
        //acc->hash;
        strcpy(acc->salt,gen_salt());

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
    //acc->hash;
    strcpy(acc->salt,gen_salt());

    return 0;
}