#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include  <stdio.h>
#include  <signal.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "../Common/constants.h"
#include "../Common/sope.h"
#include "../Common/error.h"
#include "../Common/types.h"

#include "ust_utils.h"

void _print_usage (FILE *stream);
int _parse_input (tlv_request_t *request, char *argv[]);
void _alarm_handler (int sig);

ret_code_t ret;

int main (int argc, char *argv[]) {
  tlv_request_t request;
  tlv_reply_t reply;
  argv [1] = "";
  int ulog, srvFifo,usrFIFO;
  char str_pid[5];
  struct sigaction action;

  if (argc != 6) {
    _print_usage(stderr);
    exit(ARG_ERR);
  }

  parse_input (&request, argv);

  //install handler 
  action.sa_handler = _alarm_handler;
  sigemptyset (&action.sa_mask); //all signals are delivered
  action.sa_flags = 0;
  sigaction (SIGALRM, &action, NULL); 

  //Open ulog file
  ulog = open (USER_LOGFILE, O_WRONLY | O_CREAT | O_APPEND, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
  if (ulog == -1)
    exit (FILE_OPEN_ERR);

  //create reply fifo
  sprintf (str_pid, "%d", getpid());
  if (mkfifo (strcat(SERVER_FIFO_PATH, str_pid), 0660) != 0) {
    exit (MKFIFO_ERR);
  }
  
  if ((usrFIFO = open (SERVER_FIFO_PATH, O_RDONLY)) == -1) {
    ret = RC_SRV_DOWN;
    exit (ret);
  }

  if ((srvFifo = open (SERVER_FIFO_PATH, O_WRONLY)) == -1) {
    ret = RC_SRV_DOWN;
    exit (ret);
  }

  // Send to server
  if ((write (srvFifo, &request, sizeof(request))) != 0)
    exit (FIFO_READ_ERR);

  //receive reply
  //program ends when reads from fifo or fifo timout pases 30s alarm?? yes
  alarm(FIFO_TIMEOUT_SECS);

  while (1) {
    if (ret == RC_SRV_TIMEOUT)
      break;
    if ((read(usrFIFO, &reply, sizeof (reply))) != 0)
      break;
  }

  //close
  if ((unlink(strcat (SERVER_FIFO_PATH,str_pid))) != 0)
    exit(UNLINK_ERR);

  if (close(ulog) != 0)
    exit(FILE_CLOSE_ERR);
}

void _print_usage (FILE *stream) {
  fprintf (stream, "Usage: user <acc_id> <\"acc_pass\"> <delay> <action> <\"info\">\n");
  fprintf (stream, "Action:\t0 - Create account: acc_id must be admin, info \"new_acc_id password money\"\n");
  fprintf (stream, "\t1 - Check balance: acc_id account to check, info empty \"\"\n");
  fprintf (stream, "\t2 - Money wire: acc_id account of origin, info \"dest_acc_id amount\"\n");
  fprintf (stream, "\t3 - Server shutdown: acc_id must be admin, info empty \"\"\n");
}

void _alarm_handler (int signo) {
  int i = signo;
  i++; //we should probably remove -Werror flag.
  ret=RC_SRV_TIMEOUT;
}