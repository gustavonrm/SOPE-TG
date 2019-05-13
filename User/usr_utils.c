#include "ust_utils.h"

// TODO must be tested, not sure if it is right
int parse_input (tlv_request_t *request, char *argv[]) {
  req_value_t value;
  op_type_t type;
  usleep(100000); // measure this stuff time

  value.header.pid = getpid();
  value.header.account_id = (uint32_t)atoi (argv[1]);
  strcpy (value.header.password, argv[2]);
  value.header.op_delay_ms = (uint32_t)atoi (argv[3]);

  switch (atoi (argv[4])) {
  case 0:
    type = OP_CREATE_ACCOUNT;
    value.create.account_id = (uint32_t)atoi (strtok (argv[5], " "));
    value.create.balance = (uint32_t)atoi (strtok(NULL, " "));
    strcpy (value.create.password, strtok(NULL, " "));
    break;

  case 1:
    type = OP_BALANCE;
    break;

  case 2:
    type = OP_TRANSFER;
    value.transfer.account_id = (uint32_t)atoi (strtok (argv[5], " "));
    value.transfer.amount = (uint32_t)atoi (argv[5]);
    break;

  case 3:
    type = OP_SHUTDOWN;
    break;
  // Missing default action in case of invalid input
  default:
    break;
  }

  request->length = sizeof(request);
  request->type = type;
  request->value = value; // Como está numa função isto não funciona, perde-se o value quando sair da função, tem que se allocar dinamincamente I Think
  
  return 0;
}