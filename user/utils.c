#include "utils.h"

void gen_salt (char *dst) {
  char salt[SALT_LEN +1];

  for (int i = 0; i < SALT_LEN; i++) {
    salt[i] = "0123456789abcdef" [random () % 16];
  }
  salt[SALT_LEN] = '\0';

  strcpy (dst, salt);
}

void get_hash (char *str, char *dst) {
  int pipefd[2];
  int pid;
  char buf [64 +1];

  char cmd[SALT_LEN + strlen(str) + 8];
  strcpy (cmd, "echo -n ");
  strcat (cmd, str);

  pipe(pipefd);
  pid = fork();

  if (pid == 0){
    dup2 (pipefd[1], STDOUT_FILENO);
    close (pipefd[0]);
    
    FILE *echo_proc, *sha_proc;

    echo_proc = popen (cmd, "r");
    sha_proc = popen ("sha256sum", "w");

    while (fgets(buf, 64, echo_proc) != NULL)
      fprintf(sha_proc, "%s", buf);

    pclose (echo_proc);
    pclose (sha_proc);

  } else {
    close (pipefd[1]);

    read (pipefd[0], buf, 64);
    waitpid (pid, NULL, 0);
  }
  
  strncat(dst, buf, 64);
}