#include "reply.h"

tlv_reply_t makeReply(tlv_request_t *request, uint32_t data){
    tlv_reply_t reply;

    switch (request->type)
    {
    case OP_BALANCE:
        reply.value.balance.balance=data;
        break;
    case OP_TRANSFER:
        reply.value.transfer.balance=data;
        break;
    case OP_SHUTDOWN:
        reply.value.shutdown.active_offices=data;
        break;
    default:
        break;
    }

    reply.value.header.account_id = request->value.header.account_id;
    reply.value.header.ret_code = RC_OK;
    reply.type=request->type;
    reply.length= sizeof(reply.value);

    return reply;
}

tlv_reply_t makeErrorReply(tlv_request_t *request, ret_code_t ret){
    tlv_reply_t errorReply;

    errorReply.value.header.account_id=request->value.header.account_id;
    errorReply.value.header.ret_code = ret;
    errorReply.type=request->type;
    errorReply.length = sizeof(errorReply.value);

    return errorReply;
}