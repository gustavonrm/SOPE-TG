#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../Common/types.h"
#include "../Common/error.h"
#include "../Common/sope.h"

tlv_reply_t makeReply(tlv_request_t *request, uint32_t data);

tlv_reply_t makeErrorReply(tlv_request_t *request, ret_code_t ret);