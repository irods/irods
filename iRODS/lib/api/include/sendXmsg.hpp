/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* sendXmsg.h
 *
 */

#ifndef SEND_XMSG_HPP
#define SEND_XMSG_HPP

/* This is Object File I/O type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "dataObjInpOut.hpp"
#include "msParam.hpp"

#if defined(RODS_SERVER)
#define RS_SEND_XMSG rsSendXmsg
/* prototype for the server handler */
int
rsSendXmsg( rsComm_t *rsComm, sendXmsgInp_t *sendXmsgInp );
#else
#define RS_SEND_XMSG NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
    /* prototype for the client call */
    int
    rcSendXmsg( rcComm_t *conn, sendXmsgInp_t *sendXmsgInp );

#ifdef __cplusplus
}
#endif
#endif	/* SEND_XMSG_H */
