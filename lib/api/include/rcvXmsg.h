#ifndef RCV_XMSG_H__
#define RCV_XMSG_H__

#include "rcConnect.h"
#include "rodsXmsg.h"

#ifdef __cplusplus
extern "C"
#endif
int rcRcvXmsg( rcComm_t *conn, rcvXmsgInp_t *rcvXmsgInp, rcvXmsgOut_t **rcvXmsgOut );

#endif
