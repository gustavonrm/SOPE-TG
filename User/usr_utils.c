#include "usr_utils.h"

int parse_input (tlv_request_t *request, char *argv[]) {
  req_value_t value;
  op_type_t type;

  value.header.pid = getpid ();
  value.header.account_id = (uint32_t)atoi (argv[1]);
  strcpy (value.header.password, argv[2]);
  value.header.op_delay_ms = (uint32_t)atoi (argv[3]);

  switch (atoi (argv[4])) {
  case OP_CREATE_ACCOUNT:
    type = OP_CREATE_ACCOUNT;
    value.create.account_id = (uint32_t)atoi (strtok (argv[5], " "));
    value.create.balance = (uint32_t)atoi (strtok(NULL, " "));
    strcpy (value.create.password, strtok (NULL, " "));
    break;

  case OP_BALANCE:
    type = OP_BALANCE;
    break;

  case OP_TRANSFER:
    type = OP_TRANSFER;
    value.transfer.account_id = (uint32_t)atoi (strtok (argv[5], " "));
    value.transfer.amount = (uint32_t)atoi (argv[5]);
    break;

  case OP_SHUTDOWN:
    type = OP_SHUTDOWN;
    break;
  
  default:
    break;
  }

  request->type = type;
  request->value = value;
  request->length = sizeof (request->value);

  return 0;
}

int sendRequest (tlv_request_t request, int uLog) {
  logRequest (uLog, getpid(), &request);

  printf ("L: %d\n", request.length);
  int srvFifo = open (SERVER_FIFO_PATH, O_WRONLY | O_NONBLOCK);
  if (srvFifo == -1) {
    perror ("Error opening server fifo!");
    return -1;
  }

  int nBytes = write (srvFifo, &request, sizeof (op_type_t) + sizeof (uint32_t) + request.length);
  if (nBytes == -1) {
    perror ("Error sending request to server");
    return -1;
  }

  close (srvFifo);
  
  return 0;
}