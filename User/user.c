#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "../Common/constants.h"
#include "../Common/sope.h"
#include "../Common/error.h"
#include "../Common/types.h"

#include "usr_utils.h"

void _print_usage (FILE *stream);

/////GLOBAL////
int ulogFd;
char USER_FIFO_PATH[USER_FIFO_PATH_LEN];

int main (int argc, char *argv[]) {
  tlv_request_t request;
  tlv_request_t reply;
  int usrFIFO;
  int ret;

  if (argc != 6) {
    _print_usage (stderr);
    exit (ARG_ERR);
  }
  
  if (atoi(argv[1]) > MAX_BANK_ACCOUNTS)
    return INVALID_INPUT_ERR;

  parse_input (&request, argv);

  ulogFd = open (USER_LOGFILE, O_WRONLY | O_CREAT | O_APPEND, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
  if (ulogFd == -1)
    exit (FILE_OPEN_ERR);

  ret = writeToFifo (request);
  if (ret != 0)
    printf ("Error: %s\n", ret == FIFO_OPEN_ERR ? "Opening server fifo" : "Writing to fifo");
    
  logRequest (ulogFd, getpid(), &request);

  //create reply fifo
  sprintf (USER_FIFO_PATH, "%s%d", USER_FIFO_PATH_PREFIX, getpid ());

  if (mkfifo (USER_FIFO_PATH, 0660) != 0)
    exit (MKFIFO_ERR);

  alarm (FIFO_TIMEOUT_SECS);

  //receive reply
  if ((usrFIFO = open (USER_FIFO_PATH, O_RDONLY)) == -1) {
    ret = RC_SRV_DOWN;
    exit (ret);
  }

  if ((read (usrFIFO, &reply, sizeof (reply))) != 0)
    exit (FIFO_READ_ERR);

  //close
  if (close (ulogFd) != 0)
    exit (FILE_CLOSE_ERR);

  if ((unlink (USER_FIFO_PATH) != 0))
    exit (UNLINK_ERR);
  
  close (ulogFd);

  return 0;
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
  printf ("alarm recieved\n");
  //close
  if (close (ulogFd) != 0)
    exit (FILE_CLOSE_ERR);

  if ((unlink (USER_FIFO_PATH) != 0))
    exit (UNLINK_ERR);

  exit (RC_SRV_TIMEOUT);
} 