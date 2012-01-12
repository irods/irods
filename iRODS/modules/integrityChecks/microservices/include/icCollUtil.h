/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* icCollUtil.h
 */

#ifndef _IC_COLL_UTIL
#define _IC_COLL_UTIL


#include "rods.h"
#include "rcMisc.h"
#include "rodsClient.h"


int msiListColl (msParam_t* collectionname, msParam_t* buf, ruleExecInfo_t* rei);
int msiVerifySubCollOwner (msParam_t* collinp, msParam_t* ownerinp, msParam_t *bufout, msParam_t* statout);
int msiVerifySubCollAVU (msParam_t* collinp, msParam_t* avuname, msParam_t* avuvalue, msParam_t* avuattr, msParam_t *bufout, msParam_t* statout);
int msiVerifySubCollACL (msParam_t* collinp, msParam_t* acltypeinp, msParam_t* aclnameinp, msParam_t *bufout, msParam_t* statout);

#endif	/* _IC_COLL_UTIL */
