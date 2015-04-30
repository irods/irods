/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* irodsXmsgServer.h - header file for irodsXmsgServer
 */



#ifndef XMSG_SERVER_HPP
#define XMSG_SERVER_HPP

#include "rods.h"
#include "rsGlobalExtern.hpp"   /* server global */
#include "rcGlobalExtern.hpp"     /* client global */
#include "rsLog.hpp"
#include "rodsLog.h"
#include "sockComm.h"
#include "getRodsEnv.h"
#include "rcConnect.h"

#define v_FLAG  0x1

int
xmsgServerMain();
int usage( char *prog );
#endif	/* XMSG_SERVER_H */
