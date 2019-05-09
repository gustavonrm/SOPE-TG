#include "utils.h"

void gen_salt (char *dst) {
  char salt[SALT_LEN +1];

  for (int i = 0; i < SALT_LEN; i++) {
    salt[i] = "0123456789abcdef" [random () % 16];
  }
  salt[SALT_LEN] = '\0';

  strcpy (dst, salt);
}