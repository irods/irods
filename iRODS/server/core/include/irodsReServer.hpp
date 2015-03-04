/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* irodsReServer.h - header file for irodsReServer
 */



#ifndef RE_SERVER_HPP
#define RE_SERVER_HPP

#include "rods.hpp"
#include "rcGlobalExtern.hpp"     /* client global */
#include "rsLog.hpp"
#include "rodsLog.hpp"
#include "sockComm.hpp"
#include "rsMisc.hpp"
#include "getRodsEnv.hpp"
#include "rcConnect.hpp"

#define RE_SERVER_SLEEP_TIME    30
#define RE_SERVER_EXEC_TIME     120

uint CoreIrbTimeStamp = 0;

/* definition for flagval flags */

#define v_FLAG  0x1

void
reServerMain( rsComm_t *rsComm, char* logDir );
int
reSvrSleep( rsComm_t *rsComm );
int
chkAndResetRule();
#endif	/* RE_SERVER_H */
