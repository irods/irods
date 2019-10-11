/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rodsAgent.h - header file for rodsAgent
 */

#ifndef RODS_AGENT_HPP
#define RODS_AGENT_HPP

#include "rods.h"
#include "rcGlobalExtern.h"     /* client global */
#include "rodsLog.h"
#include "sockComm.h"
#include "getRodsEnv.h"
#include "rcConnect.h"

#define MAX_MSG_READ_RETRY	1
#define READ_RETRY_SLEEP_TIME	1

int agentMain( rsComm_t *rsComm );

#endif	/* RODS_AGENT_H */
