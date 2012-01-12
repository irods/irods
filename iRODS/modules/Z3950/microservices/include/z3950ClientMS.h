/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file  z3950ClientMS.h
 *
 */



#ifndef Z3950CLIENTMS_H
#define Z3950CLIENTMS_H

#include "rods.h"
#include "reGlobalsExtern.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"

int msiz3950Submit(msParam_t *serverName,msParam_t *query,msParam_t *recordSyntax,msParam_t *outParam,ruleExecInfo_t *rei);


#endif  /* Z3950CLIENTMS_H */
