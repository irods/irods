/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#ifndef RCV_XMSG_H__
#define RCV_XMSG_H__

/* This is Object File I/O type API call */

#include "rcConnect.h"
#include "rodsXmsg.h"

#if defined(RODS_SERVER)
#define RS_RCV_XMSG rsRcvXmsg
/* prototype for the server handler */
int
rsRcvXmsg( rsComm_t *rsComm, rcvXmsgInp_t *rcvXmsgInp,
           rcvXmsgOut_t **rcvXmsgOut );
#else
#define RS_RCV_XMSG NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */
int
rcRcvXmsg( rcComm_t *conn, rcvXmsgInp_t *rcvXmsgInp,
           rcvXmsgOut_t **rcvXmsgOut );

#ifdef __cplusplus
}
#endif
#endif	// RCV_XMSG_H__
