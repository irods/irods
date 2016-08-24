#ifndef SEND_XMSG_H__
#define SEND_XMSG_H__

#include "rcConnect.h"
#include "rodsXmsg.h"

#ifdef __cplusplus
extern "C"
#endif
int rcSendXmsg( rcComm_t *conn, sendXmsgInp_t *sendXmsgInp );

#endif
