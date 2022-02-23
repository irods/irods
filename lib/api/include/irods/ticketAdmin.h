#ifndef IRODS_TICKET_ADMIN_H
#define IRODS_TICKET_ADMIN_H

#include "irods/objInfo.h"

struct RcComm;

typedef struct TicketAdminInput {
    char *arg1;
    char *arg2;
    char *arg3;
    char *arg4;
    char *arg5;
    char *arg6;
    struct KeyValPair condInput;
} ticketAdminInp_t;

#define ticketAdminInp_PI "str *arg1; str *arg2; str *arg3; str *arg4; str *arg5; str *arg6; struct KeyValPair_PI;"

#ifdef __cplusplus
extern "C"
#endif
int rcTicketAdmin(struct RcComm* conn, struct TicketAdminInput* ticketAdminInp);

#endif // IRODS_TICKET_ADMIN_H
