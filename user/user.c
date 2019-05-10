#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "constants.h"
#include "sope.h"
#include "error.h"

void _print_usage (FILE *stream);

int main (int argc, char *argv[]) {
  if (argc != 5) {
    _print_usage(stderr);
    exit (ARG_ERR);
  }
  argv[1] = "";
  int ulog, srvFifo;
  
  ulog = open (USER_LOGFILE, O_WRONLY | O_CREAT | O_APPEND, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
  if (ulog == -1)
    exit (FILE_OPEN_ERR);

  srvFifo = open (SERVER_FIFO_PATH, O_WRONLY);
  if (srvFifo == -1)
    exit (FIFO_OPEN_ERR);
  
}
void _print_usage (FILE *stream) {
  fprintf (stream, "\nUsage: user <acc_id> <\"acc_pass\"> <delay> <action> <\"info\">\n");
  fprintf (stream, "Action:\t0 - Create account: acc_id must be admin, info \"new_acc_id password money\"\n");
  fprintf (stream, "\t1 - Check balance: acc_id account to check, info empty \"\"\n");
  fprintf (stream, "\t2 - Money wire: acc_id account of origin, info \"dest_acc_id amount\"\n");
  fprintf (stream, "\t3 - Server shutdown: acc_id must be admin, info empty \"\"\n");
}