/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* irodsXmsgServer.h - header file for irodsXmsgServer
 */



#ifndef IRODS_XMSG_SERVER_H
#define IRODS_XMSG_SERVER_H

#include "rods.h"
#include "rsGlobalExtern.h"   /* server global */
#include "rcGlobalExtern.h"     /* client global */
#include "rsLog.h" 
#include "rodsLog.h"
#include "sockComm.h"
#include "rsMisc.h"
#include "getRodsEnv.h"
#include "rcConnect.h"
#include "initServer.h"

#define v_FLAG  0x1

int
xmsgServerMain ();
int usage (char *prog);
#endif	/* IRODS_XMSG_SERVER_H */
