/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rodsAgent.h - header file for rodsAgent
 */



#ifndef RODS_AGENT_H
#define RODS_AGENT_H

#include "rods.h"
#include "rsGlobal.h"   /* server global */
#include "rcGlobalExtern.h"     /* client global */
#include "rsLog.h"
#include "rodsLog.h"
#include "sockComm.h"
#include "rsMisc.h"
#include "getRodsEnv.h"
#include "rcConnect.h"
#include "initServer.h"

#define MAX_MSG_READ_RETRY	1	
#define READ_RETRY_SLEEP_TIME	1	

int agentMain (rsComm_t *rsComm);

#endif	/* RODS_AGENT_H */
