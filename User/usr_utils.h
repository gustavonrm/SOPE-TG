#ifndef _USR_UTILS_H
#define _USR_UTILS_H

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "../Common/types.h"
#include "../Common/sope.h"

int parse_input (tlv_request_t *request, char *argv[]);

int sendRequest (tlv_request_t request, int uLog);

#endif