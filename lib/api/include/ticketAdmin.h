#ifndef TICKET_ADMIN_H__
#define TICKET_ADMIN_H__

#include "rcConnect.h"

typedef struct {
    char *arg1;
    char *arg2;
    char *arg3;
    char *arg4;
    char *arg5;
    char *arg6;
} ticketAdminInp_t;
#define ticketAdminInp_PI "str *arg1; str *arg2; str *arg3; str *arg4; str *arg5; str *arg6;"


#ifdef __cplusplus
extern "C"
#endif
int rcTicketAdmin( rcComm_t *conn, ticketAdminInp_t *ticketAdminInp );

#endif
