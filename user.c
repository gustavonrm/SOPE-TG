#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "constants.h"
#include "sope.h"
#include "error.h"
#include "types.h"
#include "reply.h"

#include "usr_utils.h"

void _print_usage (FILE *stream);
void _alarm_handler ();
void _install_handler ();
void _cleanUp();

/////GLOBAL////
int ulogFd;
int usrFIFO;
char USER_FIFO_PATH[USER_FIFO_PATH_LEN];
tlv_request_t request;

int main (int argc, char *argv[]) {
  tlv_reply_t reply;
  int ret;

  if (argc != 6) {
    _print_usage (stderr);
    exit (ARG_ERR);
  }
  
  if (atoi (argv[1]) > MAX_BANK_ACCOUNTS)
    return INVALID_INPUT_ERR;

  void _install_handler ();

  parse_input (&request, argv);

  ulogFd = open (USER_LOGFILE, O_WRONLY | O_CREAT | O_APPEND, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
  if (ulogFd == -1) {
    perror (strerror (errno));
    exit (FILE_OPEN_ERR);
  }

  ret = usr_writeToFifo (request);
  if (ret != RC_OK){
    reply = makeErrorReply (&request, ret);
    logReply (ulogFd, getpid (), &reply);
    exit (FIFO_WRITE_ERR);
  }
    
  logRequest (ulogFd, getpid (), &request);

  sprintf (USER_FIFO_PATH, "%s%d", USER_FIFO_PATH_PREFIX, getpid ());

  if (mkfifo (USER_FIFO_PATH, 0660) != 0)
    exit (MKFIFO_ERR);

  alarm (FIFO_TIMEOUT_SECS);

  usrFIFO = open (USER_FIFO_PATH, O_RDONLY);
  if (usrFIFO == -1) {
    reply = makeErrorReply (&request, RC_SRV_DOWN);
    logReply (ulogFd, getpid (), &reply);
    exit (FIFO_OPEN_ERR);
  }

  reply = usr_readFifo (usrFIFO);
  logReply (ulogFd, getpid (), &reply);

  if (close (ulogFd) != 0)
    exit (FILE_CLOSE_ERR);
  
  if (close (usrFIFO) != 0)
    return FIFO_CLOSE_ERR;

  if (unlink (USER_FIFO_PATH) != 0)
    exit (UNLINK_ERR);

  return 0;
}

void _print_usage (FILE *stream) {
  fprintf (stream, "Usage: user <acc_id> \"<acc_pass>\" <delay> <action> \"<info>\"\n");
  fprintf (stream, "Action:\t0 - Create account: acc_id must be admin, info \"new_acc_id money password\"\n");
  fprintf (stream, "\t1 - Check balance: acc_id account to check, info empty \"\"\n");
  fprintf (stream, "\t2 - Money wire: acc_id account of origin, info \"dest_acc_id amount\"\n");
  fprintf (stream, "\t3 - Server shutdown: acc_id must be admin, info empty \"\"\n");
}

void _install_handler () {
  struct sigaction action;
  action.sa_handler = _alarm_handler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;
  sigaction (SIGALRM, &action, NULL);
}

void _alarm_handler () {
  if (close (ulogFd) != 0)
    exit (FILE_CLOSE_ERR);

  if (close (usrFIFO) != 0)
    exit (FIFO_CLOSE_ERR);

  if ((unlink (USER_FIFO_PATH) != 0))
    exit (UNLINK_ERR);

  tlv_reply_t reply;
  reply = makeErrorReply (&request, RC_SRV_TIMEOUT);
  logReply (ulogFd, getpid (), &reply);
  
  exit (RC_SRV_TIMEOUT);
} 