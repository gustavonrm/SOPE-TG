#include "usr_utils.h"

int parse_input (tlv_request_t *request, char *argv[]) {
  req_value_t value;
  op_type_t type;

  value.header.pid = getpid ();

  if (atoi(argv[1]) < 0 || atoi(argv[1]) > MAX_BANK_ACCOUNTS)
    exit (ARG_ERR);

  if (strlen (argv[2]) < MIN_PASSWORD_LEN || strlen (argv[2]) > MAX_PASSWORD_LEN)
    exit (ARG_ERR);

  if (atoi (argv[3]) < 0)
    exit (ARG_ERR);

  value.header.account_id = (uint32_t)atoi (argv[1]);
  strcpy (value.header.password, argv[2]);
  value.header.op_delay_ms = (uint32_t)atoi (argv[3]);
  
  switch (atoi (argv[4])) {
  case OP_CREATE_ACCOUNT: 
  {
    type = OP_CREATE_ACCOUNT;
    int id = atoi (strtok(argv[5], " "));
    if (id < 0 || id > MAX_BANK_ACCOUNTS)
      exit (ARG_ERR);
    
    uint32_t bal = (uint32_t)atoi (strtok(NULL, " "));
    if (bal < MIN_BALANCE || bal > MAX_BALANCE)
      exit (ARG_ERR);
    
    char *pass = strtok (NULL, " ");
    if (pass == NULL || strlen (pass) < MIN_PASSWORD_LEN || strlen (pass) > MAX_PASSWORD_LEN)
      exit (ARG_ERR);
    
    value.create.account_id = (uint32_t)id;
    value.create.balance = (uint32_t)bal;
    strcpy (value.create.password, pass);

    break;
  }

  case OP_BALANCE:
    type = OP_BALANCE;
    if (strlen (argv[5]) != 0)
      exit (ARG_ERR);
    break;

  case OP_TRANSFER:
  {
    type = OP_TRANSFER;
    
    int id = atoi (strtok(argv[5], " "));
    if (id < 0 || id > MAX_BANK_ACCOUNTS)
      exit (ARG_ERR);

    int amount = atoi (strtok(NULL," "));
    if (amount < 0)
      exit (ARG_ERR);

    value.transfer.account_id = (uint32_t)id;
    value.transfer.amount = (uint32_t)amount;

    break;
  }

  case OP_SHUTDOWN:
    type = OP_SHUTDOWN;
    if (strlen (argv[5]) != 0)
      exit (ARG_ERR);
    
    break;

  default:
    exit (ARG_ERR);
    break;
  }

  request->type = type;
  request->value = value;
  request->length = sizeof (request->value);

  return 0;
}

ret_code_t usr_writeToFifo (tlv_request_t request) {
  int srvFifo = open (SERVER_FIFO_PATH, O_WRONLY | O_NONBLOCK);
  if (srvFifo == -1)
    return RC_SRV_DOWN;

  int nBytes = write (srvFifo, &request, sizeof (op_type_t) + sizeof (uint32_t) + request.length);
  if (nBytes == -1)
    return RC_OTHER;

  close(srvFifo);

  return RC_OK;
}

tlv_reply_t usr_readFifo (int tmpFifo) {
  int nBytes = 0;
  tlv_reply_t reply;

  nBytes = read (tmpFifo, &reply, sizeof (op_type_t) + sizeof (uint32_t));
  if (nBytes == -1)
    exit(FIFO_READ_ERR);

  nBytes = read (tmpFifo, &reply.value, reply.length);
  if (nBytes == -1)
    exit (FIFO_READ_ERR);

  if (nBytes == 0)
    exit (FIFO_READ_ERR);

  return reply;
}

int verifyIfInt(char* string){
  for (unsigned int i=0; i<strlen(string); i++){
    if(!isdigit(string[i]))
      return -1;
  }
  return 0;
}

void print_reply (tlv_reply_t reply) {
  printf ("acc id: %d\n", reply.value.header.account_id);
  printf ("ret: %d\n", reply.value.header.ret_code);
}