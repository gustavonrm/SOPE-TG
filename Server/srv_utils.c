#include "srv_utils.h"

queue_t requestQueue;

void gen_salt(char *salt)
{
  srand(time(NULL));

  char nSalt[SALT_LEN + 1];
  static char charSet[] = "0123456789abcdef";

  for (int i = 0; i < SALT_LEN; i++)
    nSalt[i] = charSet[rand() % (int)(sizeof(charSet) - 1)];

  nSalt[SALT_LEN] = '\0';

  strcpy(salt, nSalt);
}

void get_hash(char *str, char *hash)
{
  int pipefd[2];
  int pid;
  char buf[64 + 1];

  char cmd[SALT_LEN + strlen(str) + 8];
  strcpy(cmd, "echo -n ");
  strcat(cmd, str);

  pipe(pipefd);
  pid = fork();

  if (pid == 0)
  {
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[0]);

    FILE *echo_proc, *sha_proc;

    echo_proc = popen(cmd, "r");
    sha_proc = popen("sha256sum", "w");

    while (fgets(buf, 64, echo_proc) != NULL)
      fprintf(sha_proc, "%s", buf);

    pclose(echo_proc);
    pclose(sha_proc);

    exit(0);
  }
  else
  {
    close(pipefd[1]);

    read(pipefd[0], buf, 64);
    waitpid(pid, NULL, 0);
  }

  strncat(hash, buf, 64);
}
//int readFifo (int srvFifo)
int readFifo(int srvFifo, sem_t full, sem_t empty, pthread_mutex_t mut)
{
  int nBytes = 0;

  while (1)
  {
    tlv_request_t request;

    nBytes = read(srvFifo, &request, sizeof(op_type_t) + sizeof(uint32_t));
    if (nBytes == -1)
      exit(FIFO_READ_ERR);

    if (nBytes == 0)
      break;

    nBytes = read(srvFifo, &request.value, request.length);
    if (nBytes == -1)
      exit(FIFO_READ_ERR);

    if (nBytes == 0)
      break;

    printf("li coisas\n");

    sem_wait(&empty);
    pthread_mutex_lock(&mut);
    printf("producer\n");
    queuePush(request);
    // Detetar thread vag
    pthread_mutex_unlock(&mut);
    sem_post(&full);
  }
  return 0;
}

void queuePush(tlv_request_t request)
{
  queue_el_t *new;

  new = (queue_el_t *)malloc(sizeof(queue_el_t));
  if (new == NULL)
    return;

  new->request = request;
  new->next = NULL;

  if (requestQueue.tail == NULL)
  {
    requestQueue.head = requestQueue.tail = new;
  }
  else
  {
    requestQueue.tail->next = new;
    requestQueue.tail = new;
  }
}

tlv_request_t queuePop()
{
  queue_el_t *first;

  first = requestQueue.head;
  requestQueue.head = first->next;

  if (requestQueue.head == NULL)
    requestQueue.tail = NULL;

  tlv_request_t ret;
  ret = first->request;
  free(first);

  return ret;
}

void queueDelete()
{
  queue_el_t *aux;

  while (requestQueue.head != NULL)
  {
    aux = requestQueue.head->next;
    free(requestQueue.head);
    requestQueue.head = aux;
  }
}

tlv_reply_t makeReply(tlv_request_t *request, uint32_t data){
  tlv_reply_t reply;

  switch (request->type){
    case OP_BALANCE:
      reply.value.balance.balance=data;
      break;
    case OP_TRANSFER:
      reply.value.transfer.balance=data;
      break;
    case OP_SHUTDOWN:
      reply.value.shutdown.active_offices=data;
      break;
    default:
      break;
  }

  reply.value.header.account_id = request->value.header.account_id;
  reply.value.header.ret_code = RC_OK;
  reply.type=request->type;
  reply.length = sizeof(request->value);

  return reply;
}

tlv_reply_t makeErrorReply(tlv_request_t *request, enum ret_code ret){
  tlv_reply_t errorReply;

  errorReply.value.header.account_id=request->value.header.account_id;
  errorReply.value.header.ret_code = ret;
  errorReply.type=request->type;
  errorReply.length = sizeof(request->value);

  return errorReply;
}

int writeToFifo (tlv_reply_t reply,char *path) {
  int tmpFifo = open (path, O_WRONLY | O_NONBLOCK);
  if (tmpFifo == -1)
    return FIFO_OPEN_ERR;

  int nBytes = write (tmpFifo, &reply, sizeof (op_type_t) + sizeof (uint32_t) + reply.length);
  if (nBytes == -1)
    return FIFO_WRITE_ERR;

  close (tmpFifo);
  
  return 0;
}
  
void print_request(tlv_request_t request)
{
  printf("acc id: %d", request.value.header.account_id);
  printf("pass: %s", request.value.header.password);
  printf("delay: %d", request.value.header.op_delay_ms);
  printf("aac id create: %d", request.value.create.account_id);
  printf("balance: %d", request.value.create.balance);
  printf("balance: %s", request.value.create.password);
}