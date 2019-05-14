#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>

#include "../Common/constants.h"
#include "../Common/types.h"

void gen_salt (char *salt);

void get_hash (char *str, char *hash);

void readRequest (int srvFifo);
