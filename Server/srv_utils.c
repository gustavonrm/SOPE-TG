#include "srv_utils.h"

void gen_salt (char *salt) {
  char nSalt[SALT_LEN +1];
  static char charSet[] = "0123456789abcdef";

  for (int i = 0; i < SALT_LEN; i++)
    nSalt[i] = charSet[rand() % (int) (sizeof (charSet) -1)];

  nSalt[SALT_LEN] = '\0';
  
  strcpy (salt, nSalt);
}

void get_hash (char *str, char *hash) {
  int pipefd[2];
  int pid;
  char buf[64 +1];

  char cmd[SALT_LEN + strlen (str) + 8];
  strcpy (cmd, "echo -n ");
  strcat (cmd, str);

  pipe (pipefd);
  pid = fork ();

  if (pid == 0){
    dup2 (pipefd[1], STDOUT_FILENO);
    close (pipefd[0]);
    
    FILE *echo_proc, *sha_proc;

    echo_proc = popen (cmd, "r");
    sha_proc = popen ("sha256sum", "w");

    while (fgets (buf, 64, echo_proc) != NULL)
      fprintf (sha_proc, "%s", buf);

    pclose (echo_proc);
    pclose (sha_proc);
    exit (0); 
  } else {
    close (pipefd[1]);

    read (pipefd[0], buf, 64);
    waitpid (pid, NULL, 0);
  }
  
  strncat (hash, buf, 64);
}

void readRequest (int srvFifo) {
  int nBytes = 0;

  while (1){
    tlv_request_t request;

    nBytes = read (srvFifo, &request, sizeof (op_type_t) + sizeof(uint32_t));
    if (nBytes == -1) {
      perror ("Error reading request type and length");
      return ;
    }

    nBytes = read (srvFifo, &request.value, request.length);
    if (nBytes == -1) {
      perror ("Error reading request value");
      return ;
    }

    if (nBytes == 0)
      break;

    printf ("L: %d\n", request.length);
    printf ("T: %d\n", request.type);
    printf ("Id: %d\n", request.value.create.account_id);
    printf ("Bl: %d\n", request.value.create.balance);
    printf ("Ps: %s\n", request.value.create.password);
  }
}