#include "utils.h"

const char * gen_salt(){

    char salt[SALT_LEN];
    char* ret =malloc(SALT_LEN*sizeof(char));

    for(int i=0; i<SALT_LEN; i++){
        salt[i]="0123456789abcdef"[random () % 16];
    }
    strcpy(ret, salt);
    return ret;
}