/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file	examplesMS.c
 *
 * @brief	Hello world example.
 *
 *
 * @author	Mike Wan,  University of California, San Diego
 */

#include "rsApiHandler.h"
#include "objMetaOpr.h"
#include "resource.h"
#include "examplesMS.h"


/**
 * Hello world example.
 *
 * @param[in]		name		the name of a person 
 * @param[out]		outParam	the output hello message
 * @param[in,out]	rei		the rule execution information
 * @return				the status code, 0 on success
 */
int
msiHello( msParam_t *name, msParam_t *outParam,
	ruleExecInfo_t *rei )
{
    char *tmpPtr;
    char outStr[MAX_NAME_LEN];

    RE_TEST_MACRO( "    Calling msiHello" );

    tmpPtr = parseMspForStr (name);

    if (tmpPtr == NULL)  {
	rodsLog (LOG_ERROR, "msiHello: missing name input");
	rei->status = USER__NULL_INPUT_ERR;
	return USER__NULL_INPUT_ERR;
    }

    snprintf (outStr, MAX_NAME_LEN, "Hello world from %s", tmpPtr);
    
    fillStrInMsParam (outParam, outStr);

    rei->status = 0;
    return 0;
}


/**
 * msiGetRescAddr
 *
 * @param[in]		rescName	the resource name 
 * @param[out]		outAddress	the returned IP address
 * @param[in,out]	rei		the rule execution information
 * @return				the status code, 0 on success
 */
int
msiGetRescAddr( msParam_t *rescName, msParam_t *outAddress,
	ruleExecInfo_t *rei )
{
    char *tmpPtr;
    int status;
    rescGrpInfo_t *rescGrpInfo = NULL;

    RE_TEST_MACRO( "    Calling msiGetRescAddr" );

    tmpPtr = parseMspForStr (rescName);

    if (tmpPtr == NULL)  {
	rodsLog (LOG_ERROR, "msiGetRescAddr: missing name input");
	rei->status = USER__NULL_INPUT_ERR;
	return USER__NULL_INPUT_ERR;
    }

    status = _getRescInfo (rei->rsComm, tmpPtr, &rescGrpInfo);
    if (status < 0) {
         rodsLog (LOG_ERROR,
          "msiGetRescAddr: _getRescInfo of %s error. stat = %d",
          rescName, status);
        return status;
    }

    fillStrInMsParam (outAddress, rescGrpInfo->rescInfo->rescLoc);

    rei->status = 0;
    return 0;
}

