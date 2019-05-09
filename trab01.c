#include "trab01.h"

void get_hash (char *str, char *dst) {
  if (str == NULL)
    exit(ARG_ERR);

  int link[2];
  pid_t pid;
  char buf[HASH_LEN + 1];
  
  char *cmd = NULL;
  strcat (cmd, "echo -n ");
  strcat (cmd, str);
  strcat (cmd, " | sha256sum");

  if (pipe (link) == -1)
    exit (PIPE_OPEN_ERR);

  pid = fork ();
  if (pid == -1)
    exit (FORK_ERR);

  if (pid == 0) { // Child
    dup2 (link[1], STDOUT_FILENO);
    close (link[0]);

    execlp(cmd, cmd, NULL);

  } else { // Parent
    close (link[1]);

    read (link[0], buf, HASH_LEN +1);
    
    waitpid (pid, NULL, 0);
  }

  buf[strcspn (buf, " ")] = '\0'; //Deletes \n at the end

  strcpy (dst, buf);
}