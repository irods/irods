/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* ticketAdmin.h
 */

/* This client/server call is used for ticket-based-access
   administrative functions.  It is used to create, modify, delete,
   or utilize tickets.
 */

#ifndef TICKET_ADMIN_H
#define TICKET_ADMIN_H

/* This is a Metadata type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "icatDefines.h"

typedef struct {
   char *arg1;
   char *arg2;
   char *arg3;
   char *arg4;
   char *arg5;
   char *arg6;
} ticketAdminInp_t;
    
#define ticketAdminInp_PI "str *arg1; str *arg2; str *arg3; str *arg4; str *arg5; str *arg6;"

#ifdef  __cplusplus
extern "C" {
#endif

#if defined(RODS_SERVER)
#define RS_TICKET_ADMIN rsTicketAdmin
/* prototype for the server handler */
int
rsTicketAdmin (rsComm_t *rsComm, ticketAdminInp_t *ticketAdminInp );

int
_rsTicketAdmin (rsComm_t *rsComm, ticketAdminInp_t *ticketAdminInp );
#else
#define RS_TICKET_ADMIN NULL
#endif

/* prototype for the client call */
int
rcTicketAdmin (rcComm_t *conn, ticketAdminInp_t *ticketAdminInp);

#ifdef  __cplusplus
}
#endif

#endif	/* TICKET_ADMIN_H */
