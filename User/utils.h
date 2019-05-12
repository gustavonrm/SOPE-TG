#pragma once

#include "constants.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>

void gen_salt (char *dst);

void get_hash (char *str, char *dst);


