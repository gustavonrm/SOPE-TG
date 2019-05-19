#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "error.h"
#include "sope.h"

tlv_reply_t makeReply(tlv_request_t *request, uint32_t data);

tlv_reply_t makeErrorReply(tlv_request_t *request, ret_code_t ret);