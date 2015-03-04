/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* irodsXmsgServer.h - header file for irodsXmsgServer
 */



#ifndef XMSG_SERVER_HPP
#define XMSG_SERVER_HPP

#include "rods.hpp"
#include "rsGlobalExtern.hpp"   /* server global */
#include "rcGlobalExtern.hpp"     /* client global */
#include "rsLog.hpp"
#include "rodsLog.hpp"
#include "sockComm.hpp"
#include "rsMisc.hpp"
#include "getRodsEnv.hpp"
#include "rcConnect.hpp"

#define v_FLAG  0x1

int
xmsgServerMain();
int usage( char *prog );
#endif	/* XMSG_SERVER_H */
