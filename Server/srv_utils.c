#include "srv_utils.h"

queue_t requestQueue;

void gen_salt (char *salt) {
  srand (time (NULL));

  char nSalt[SALT_LEN + 1];
  static char charSet[] = "0123456789abcdef";

  for (int i = 0; i < SALT_LEN; i++)
    nSalt[i] = charSet[rand() % (int)(sizeof (charSet) - 1)];

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
  pid = fork();

  if (pid == 0) {
    dup2 (pipefd[1], STDOUT_FILENO);
    close (pipefd[0]);

    FILE *echo_proc, *sha_proc;

    echo_proc = popen (cmd, "r");
    sha_proc = popen ("sha256sum", "w");

    while (fgets (buf, 64, echo_proc) != NULL)
      fprintf (sha_proc, "%s", buf);

    pclose (echo_proc);
    pclose (sha_proc);

    exit(0);
  } else {
    close (pipefd[1]);

    read (pipefd[0], buf, 64);
    waitpid (pid, NULL, 0);
  }

  strncat (hash, buf, 64);
}

int readFifo (int srvFifo) {
  int nBytes = 0;

  while (1) {
    tlv_request_t request;

    nBytes = read (srvFifo, &request, sizeof (op_type_t) + sizeof (uint32_t));
    if (nBytes == -1)
      exit(FIFO_READ_ERR);

    if (nBytes == 0)
      break;
    
    nBytes = read (srvFifo, &request.value, request.length);
    if (nBytes == -1)
      exit (FIFO_READ_ERR);

    if (nBytes == 0)
      break;

    //queuePush (request);
  }

  return 0;
}

void queuePush (tlv_request_t request) {
  queue_el_t *new;

  new = (queue_el_t *) malloc (sizeof (queue_el_t));
  if (new == NULL)
    return;
  
  new -> request = request;
  new -> next = NULL;
  
  if (requestQueue.tail == NULL) {
    requestQueue.head = requestQueue.tail = new;
  } else {
    requestQueue.tail -> next = new;
    requestQueue.tail = new;
  }
}

tlv_request_t queuePop (){
  queue_el_t *first;

  first = requestQueue.head;
  requestQueue.head = first -> next;

  if (requestQueue.head == NULL)
    requestQueue.tail = NULL;
  
  tlv_request_t ret;
  ret = first -> request;
  free (first);

  return ret;
}

void queueDelete () {
  queue_el_t *aux;

  while (requestQueue.head != NULL) {
    aux = requestQueue.head -> next;
    free (requestQueue.head);
    requestQueue.head = aux;
  }
}

tlv_reply_t makeReply(enum ret_code ret, tlv_request_t request){
  tlv_reply_t reply;

  reply.length=request.length;
  reply.type=request.type;
  reply.value.header.ret_code = ret;

  return reply;
}