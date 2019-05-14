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

void _print_usage(FILE *stream);
void _alarm_handler(int sig);

ret_code_t ret;
char USER_FIFO_PATH[USER_FIFO_PATH_LEN];
int ulog, srvFifo, usrFIFO;

int main(int argc, char *argv[])
{

  tlv_request_t request;
  //tlv_reply_t reply;
  argv[1] = "";
  struct sigaction action;

  if (argc != 6) {
    _print_usage (stderr);
    exit (ARG_ERR);
  }
  
  parse_input (&request, argv);

  //install handler
  action.sa_handler = _alarm_handler;
  sigemptyset(&action.sa_mask); //all signals are delivered
  action.sa_flags = 0;
  sigaction(SIGALRM, &action, NULL);

  //Open ulog file
  ulog = open(USER_LOGFILE, O_WRONLY | O_CREAT | O_APPEND, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
  if (ulog == -1)
    exit(FILE_OPEN_ERR);

  //create reply fifo
  sprintf(USER_FIFO_PATH, "%s%d", USER_FIFO_PATH_PREFIX, getpid());

  if (mkfifo(USER_FIFO_PATH, 0660) != 0)
  {
    exit(MKFIFO_ERR);
  }

  if ((srvFifo = open(SERVER_FIFO_PATH, O_WRONLY)) == -1)
  {
    ret = RC_SRV_DOWN;
    exit(ret);
  }

  // Send to server
  if ((write(srvFifo, &request, sizeof(request))) != 0)
    exit(FIFO_READ_ERR);
  //erro ta aqui idk y
  close (srvFifo);

  //receive reply
  /*if ((usrFIFO = open(USER_FIFO_PATH, O_RDONLY)) == -1)
  {
    ret = RC_SRV_DOWN;
    exit(ret);
  }
  if ((read(usrFIFO, &reply, sizeof(reply))) != 0)
    exit(FIFO_READ_ERR);

  //close
  if (close(ulog) != 0)
    exit(FILE_CLOSE_ERR);

  if ((unlink(USER_FIFO_PATH) != 0))
    exit(UNLINK_ERR);*/
  
  return 0;
}

void _print_usage(FILE *stream)
{
  fprintf(stream, "Usage: user <acc_id> <\"acc_pass\"> <delay> <action> <\"info\">\n");
  fprintf(stream, "Action:\t0 - Create account: acc_id must be admin, info \"new_acc_id password money\"\n");
  fprintf(stream, "\t1 - Check balance: acc_id account to check, info empty \"\"\n");
  fprintf(stream, "\t2 - Money wire: acc_id account of origin, info \"dest_acc_id amount\"\n");
  fprintf(stream, "\t3 - Server shutdown: acc_id must be admin, info empty \"\"\n");
}

void _alarm_handler(int signo)
{
  int i = signo;
  i++; //we should probably remove -Werror flag.
  printf("alarm recieved\n");
  //close
  if (close(ulog) != 0)
    exit(FILE_CLOSE_ERR);

  if ((unlink(USER_FIFO_PATH) != 0))
    exit(UNLINK_ERR);
  ret = RC_SRV_TIMEOUT;
  exit(ret);
}