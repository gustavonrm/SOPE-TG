#include "srv_utils.h"

queue_t requestQueue;

void gen_salt (char *salt) {
  srand (time (NULL));

  char nSalt[SALT_LEN + 1];
  static char charSet[] = "0123456789abcdef";

  for (int i = 0; i < SALT_LEN; i++)
    nSalt[i] = charSet[rand () % (int)(sizeof (charSet) - 1)];

  nSalt[SALT_LEN] = '\0';

  strcpy (salt, nSalt);
}

void get_hash (char *str, char *hash) {
  int pipefd[2];
  int pid;
  char buf[64 + 1];

  char cmd[SALT_LEN + strlen (str) + 8];
  strcpy (cmd, "echo -n ");
  strcat (cmd, str);

  pipe (pipefd);
  pid = fork ();

  if (pid == 0) {
    dup2 (pipefd[1], STDOUT_FILENO);
    close (pipefd[0]);

    FILE *echo_proc, *sha_proc;

    echo_proc = popen (cmd, "r");
    sha_proc = popen ("sha256sum", "w");

    while (fgets (buf, HASH_LEN, echo_proc) != NULL)
      fprintf (sha_proc, "%s", buf);

    pclose (echo_proc);
    pclose (sha_proc);
    buf[HASH_LEN] = '\0';
    exit (0);
  } else {
    close (pipefd[1]);

    read (pipefd[0], buf, 64);
    waitpid (pid, NULL, 0);
  }
  
  strncpy (hash, buf, 64);
}

//int readFifo (int srvFifo)
int readFifo (int srvFifo, sem_t full, sem_t empty, pthread_mutex_t mut) {
  int nBytes = 0;

  while (1) {
    tlv_request_t request;

    nBytes = read (srvFifo, &request, sizeof (op_type_t) + sizeof (uint32_t));
    if (nBytes == -1)
      exit (FIFO_READ_ERR);

    if (nBytes == 0)
      break;

    nBytes = read (srvFifo, &request.value, request.length);
    if (nBytes == -1)
      exit (FIFO_READ_ERR);

    if (nBytes == 0)
      break;

    printf ("li coisas\n");

    sem_wait (&empty);
    pthread_mutex_lock (&mut);
    printf ("producer\n");
    queuePush (request);
    // Detetar thread vag
    pthread_mutex_unlock (&mut);
    sem_post (&full);
  }
  return 0;
}

void queuePush (tlv_request_t request) {
  queue_el_t *new;

  new = (queue_el_t *)malloc (sizeof (queue_el_t));
  if (new == NULL)
    return;

  new->request = request;
  new->next = NULL;

  if (requestQueue.tail == NULL) {
    requestQueue.head = requestQueue.tail = new;
  } else {
    requestQueue.tail->next = new;
    requestQueue.tail = new;
  }
}

tlv_request_t queuePop () {
  queue_el_t *first;

  first = requestQueue.head;
  requestQueue.head = first->next;

  if (requestQueue.head == NULL)
    requestQueue.tail = NULL;

  tlv_request_t ret;
  ret = first->request;
  free (first);

  return ret;
}

void queueDelete () {
  queue_el_t *aux;

  while (requestQueue.head != NULL) {
    aux = requestQueue.head->next;
    free (requestQueue.head);
    requestQueue.head = aux;
  }
}

int writeToFifo (tlv_reply_t reply, char *path) {
  int tmpFifo = open(path, O_WRONLY | O_NONBLOCK);
  if (tmpFifo == -1)
    return FIFO_OPEN_ERR;
  
  int nBytes;
  nBytes = write (tmpFifo, &reply, sizeof (op_type_t) + sizeof (uint32_t) + reply.length);
  if (nBytes == -1)
    return FIFO_WRITE_ERR;
  

  close (tmpFifo);

  return 0;
}

void delay (tlv_request_t request) {
  switch (request.type) {
  case OP_CREATE_ACCOUNT:
    usleep (request.value.header.op_delay_ms);
    break;

  case OP_BALANCE:
    usleep (request.value.header.op_delay_ms);
    break;

  case OP_TRANSFER:
      usleep (request.value.header.op_delay_ms);
    break;

  case OP_SHUTDOWN:
    //imediatamente antes de ser impossibilitado o envio de novos pedidos
    usleep (request.value.header.op_delay_ms);
    break;

  case __OP_MAX_NUMBER:
    break;
  }
}

ret_code_t checkLogin(bank_account_t *account, uint32_t id, char password[]){
  
  char saltedPass[SALT_LEN + MAX_PASSWORD_LEN];

  strcpy (saltedPass, account[id].salt);
  strcat (saltedPass, password);

  char hash[HASH_LEN];
  get_hash (saltedPass, hash);

  if(strncmp (account[id].hash, hash, 64) != 0)
    return RC_LOGIN_FAIL;

  return RC_OK;
}

void print_request (tlv_request_t request) {
  printf ("acc id: %d\n", request.value.header.account_id);
  printf ("pass: %s\n", request.value.header.password);
  printf ("delay: %d\n", request.value.header.op_delay_ms);
  printf ("aac id create: %d\n", request.value.create.account_id);
  printf ("balance: %d\n", request.value.create.balance);
  printf ("balance: %s\n", request.value.create.password);
}