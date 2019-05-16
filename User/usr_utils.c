#include "usr_utils.h"

int verifyIfInt(char* string);

int parse_input (tlv_request_t *request, char *argv[]) {
  req_value_t value;
  op_type_t type;

  if(!verifyIfInt(argv[1]))
    return ACC_CREATE_ERR;

  value.header.pid = getpid ();
  value.header.account_id = (uint32_t)atoi (argv[1]);
  strcpy (value.header.password, argv[2]);
  value.header.op_delay_ms = (uint32_t)atoi (argv[3]);

  switch (atoi (argv[4])) {
  case OP_CREATE_ACCOUNT:
    type = OP_CREATE_ACCOUNT;
    if(!verifyIfInt(strtok (argv[5], " ")))
      return ACC_CREATE_ERR;
    value.create.account_id = (uint32_t)atoi (strtok (argv[5], " "));
    value.create.balance = (uint32_t)atoi (strtok (NULL, " "));
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

int writeToFifo (tlv_request_t request) {
  int srvFifo = open (SERVER_FIFO_PATH, O_WRONLY);
  if (srvFifo == -1)
    return FIFO_OPEN_ERR;

  int nBytes = write (srvFifo, &request, sizeof (op_type_t) + sizeof (uint32_t) + request.length);
  if (nBytes == -1)
    return FIFO_WRITE_ERR;

  close (srvFifo);
  
  return 0;
}


tlv_reply_t readFifo(int tmpFifo){
    int nBytes = 0;
    tlv_reply_t reply;

    nBytes = read(tmpFifo, &reply, sizeof(op_type_t) + sizeof(uint32_t));
    if (nBytes == -1)
      exit(FIFO_READ_ERR);

    if (nBytes == 0)
      exit(18);
    
    print_reply(reply);

    nBytes = read(tmpFifo, &reply.value, reply.length);
    if (nBytes == -1)
      exit(FIFO_READ_ERR);
    print_reply(reply);
    if (nBytes == 0)
      exit(19);
    

    printf("li coisas\n");
    return reply;
}

void print_reply(tlv_reply_t reply)
{
  printf("acc id: %d\n", reply.value.header.account_id);
  printf("ret: %d\n", reply.value.header.ret_code);
}

int verifyIfInt(char* string){
  for (unsigned int i=0; i<strlen(string); i++){
    if(!isdigit(string[i]))
      return -1;
  }
  return 0;
}