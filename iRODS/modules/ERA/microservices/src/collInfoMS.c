/**
 * @file collInfoMS.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "collInfoMS.h"
#include "eraUtil.h"



/**
 * \fn msiIsColl(msParam_t *targetPath, msParam_t *collId, msParam_t *status, ruleExecInfo_t *rei)
 *
 * \brief Checks if an iRODS path is a collection. For use in workflows.
 *
 * \module ERA
 *
 * \since 2.1
 *
 * \author  Antoine de Torcy
 * \date    2009-05-08
 *
 * \note This microservice takes an iRODS path and returns the corresponding collection ID,
 *    or zero if the object is not a collection or does not exist.
 *    Avoid path names ending with '/' as they can be misparsed by lower level routines
 *    (eg: use /tempZone/home instead of /tempZone/home/).
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] targetPath - An msParam_t of any type whose inOutStruct is a string (the object's path).
 * \param[out] collId - an INT_MS_T containing the collection ID.
 * \param[out] status - an INT_MS_T containing the operation status.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence None
 * \DolVarModified None
 * \iCatAttrDependence None
 * \iCatAttrModified None
 * \sideeffect None
 *
 * \return integer
 * \retval 0 on success
 * \pre None
 * \post None
 * \sa None
**/
int
msiIsColl(msParam_t *targetPath, msParam_t *collId, msParam_t *status, ruleExecInfo_t *rei)
{
	char *targetPathStr;				/* for parsing input path */
	rodsLong_t coll_id;					/* collection ID */
	
	
	
	/* For testing mode when used with irule --test */
	RE_TEST_MACRO ("    Calling msiIsColl")
	
	
	/* Sanity test */
	if (rei == NULL || rei->rsComm == NULL) {
			rodsLog (LOG_ERROR, "msiIsColl: input rei or rsComm is NULL.");
			return (SYS_INTERNAL_NULL_INPUT_ERR);
	}
	
	
	/* Check for NULL input */
    if (!targetPath) 
    {
    	rei->status = USER__NULL_INPUT_ERR;
		rodsLog (LOG_ERROR, "msiIsColl: input targetPath error. status = %d", rei->status);
		return (rei->status);
    }
    
	
	/* Parse targetPath input */
	targetPathStr = (char *) targetPath->inOutStruct;
	if ( !targetPathStr || !strlen(targetPathStr) || strlen(targetPathStr) > MAX_NAME_LEN )
	{
		rei->status = USER_INPUT_PATH_ERR;
		rodsLog (LOG_ERROR, "msiIsColl: input targetPath error. status = %d", rei->status);
		return (rei->status);	
	}


	/* Call isColl()*/
	rei->status = isColl (rei->rsComm, targetPathStr, &coll_id);
	
    
    /* Return 0 if no object was found */
    if (rei->status == CAT_NO_ROWS_FOUND)
    {
    	coll_id = 0;
    	rei->status = 0;
    }

	/* Return collection ID and operation status */
	fillIntInMsParam (collId, (int)coll_id);
	fillIntInMsParam (status, rei->status);

	/* Done */
	return rei->status;
}



/**
 * \fn msiIsData(msParam_t *targetPath, msParam_t *dataId, msParam_t *status, ruleExecInfo_t *rei)
 *
 * \brief Checks if an iRODS path is a data object (an iRODS file). For use in workflows.
 *
 * \module ERA
 *
 * \since 2.1
 *
 * \author  Antoine de Torcy
 * \date    2009-05-08
 *
 * \note This microservice takes an iRODS path and returns the corresponding object ID,
 *    or zero if the object is not a data object or does not exist.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] targetPath - An msParam_t of any type whose inOutStruct is a string (the object's path).
 * \param[out] dataId - an INT_MS_T containing the data object ID.
 * \param[out] status - an INT_MS_T containing the operation status.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence None
 * \DolVarModified None
 * \iCatAttrDependence None
 * \iCatAttrModified None
 * \sideeffect None
 *
 * \return integer
 * \retval 0 on success
 * \pre None
 * \post None
 * \sa None
**/
int
msiIsData(msParam_t *targetPath, msParam_t *dataId, msParam_t *status, ruleExecInfo_t *rei)
{
	char *targetPathStr;				/* for parsing input path */
	rodsLong_t data_id;					/* data object ID */
	
	char id_str[LONG_NAME_LEN];			/* placeholder for temporary mod */
	
	
	
	/* For testing mode when used with irule --test */
	RE_TEST_MACRO ("    Calling msiIsData")
	
	
	/* Sanity test */
	if (rei == NULL || rei->rsComm == NULL) {
			rodsLog (LOG_ERROR, "msiIsData: input rei or rsComm is NULL.");
			return (SYS_INTERNAL_NULL_INPUT_ERR);
	}
	
	
	/* Check for NULL input */
    if (!targetPath) 
    {
    	rei->status = USER__NULL_INPUT_ERR;
		rodsLog (LOG_ERROR, "msiIsData: input targetPath error. status = %d", rei->status);
		return (rei->status);
    }
    
	
	/* Parse targetPath input */
	targetPathStr = (char *) targetPath->inOutStruct;
	if ( !targetPathStr || !strlen(targetPathStr) || strlen(targetPathStr) > MAX_NAME_LEN )
	{
		rei->status = USER_INPUT_PATH_ERR;
		rodsLog (LOG_ERROR, "msiIsData: input targetPath error. status = %d", rei->status);
		return (rei->status);	
	}


	/* Call isData()*/
	rei->status = isData (rei->rsComm, targetPathStr, &data_id);
	
    
    /* Return 0 if no object was found */
    if (rei->status == CAT_NO_ROWS_FOUND)
    {
    	data_id = 0;
    	rei->status = 0;
    }


	
	/***********  ANT1 TMP MOD **************/
	snprintf(id_str, LONG_NAME_LEN, "%d", (int)data_id);	
	fillStrInMsParam (dataId, id_str);
	/****************************************/


	/* Return object ID and operation status */
//	fillIntInMsParam (dataId, (int)data_id);
	fillIntInMsParam (status, rei->status);

	/* Done */
	return rei->status;
}


/**
 * \fn msiGetObjectPath(msParam_t *object, msParam_t *path, msParam_t *status, ruleExecInfo_t *rei)
 *
 * \brief Returns the path of an iRODS data object. For use in workflows.
 *
 * \module ERA
 *
 * \since 2.4.x
 *
 * \author  Antoine de Torcy
 * \date    2010-07-28
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] object - A DataObjInp_MS_T, our object.
 * \param[out] path - a STR_MS_T with the object's path.
 * \param[out] status - an INT_MS_T containing the operation status.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence None
 * \DolVarModified None
 * \iCatAttrDependence None
 * \iCatAttrModified None
 * \sideeffect None
 *
 * \return integer
 * \retval 0 on success
 * \pre  none
 * \post none
 * \sa none
**/
int
msiGetObjectPath(msParam_t *object, msParam_t *path, msParam_t *status, ruleExecInfo_t *rei)
{
	dataObjInp_t dataObjInpCache, *dataObjInp;


	/* For testing mode when used with irule --test */
	RE_TEST_MACRO ("    Calling msiGetObjectPath")


	/* Sanity checks */
	if (rei == NULL || rei->rsComm == NULL)
	{
		rodsLog (LOG_ERROR, "msiGetObjectPath: input rei or rsComm is NULL.");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	/* Parse object */
	rei->status = parseMspForDataObjInp (object, &dataObjInpCache, &dataObjInp, 0);
	if (rei->status < 0)
	{
		rodsLog (LOG_ERROR, "msiGetObjectPath: input error, status = %d", rei->status);
		return (rei->status);
	}

	/* Return object path as string */
	fillStrInMsParam (path, dataObjInp->objPath);

	/* Done */
	fillIntInMsParam (status, rei->status);
	return (rei->status);
}




/**
 * \fn msiGetCollectionContentsReport(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief Returns the number of objects in a collection by data type
 *
 * \module ERA
 *
 * \since pre-2.1
 *
 * \author  Antoine de Torcy
 * \date    2008-03-17
 *
 * \note This microservice returns the number of objects for each known data type in a collection, recursively.
 *    The results are written to a KeyValPair_MS_T, with a keyword for each data type plus "unknown" for objects
 *    of unknown data type, and the number of data objects of that type.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - A CollInp_MS_T or a STR_MS_T with the irods path of the target collection.
 * \param[out] inpParam2 (poorly named...) - A KeyValPair_MS_T containing the results.
 * \param[out] outParam - an INT_MS_T containing the status.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence None
 * \DolVarModified None
 * \iCatAttrDependence None
 * \iCatAttrModified None
 * \sideeffect None
 *
 * \return integer
 * \retval 0 on success
 * \pre None
 * \post None
 * \sa None
**/
int
msiGetCollectionContentsReport(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei)
{
	collInp_t collInpCache, *outCollInp;	/* for parsing collection input param */

	keyValPair_t *contents;			/* for passing out results */

	char collQCond[2*MAX_NAME_LEN];		/* for condition in rsGenquery() */
	genQueryInp_t genQueryInp;		/* for query inputs */
	genQueryOut_t *genQueryOut;		/* for query results */

	char *resultStringToken;		/* for parsing key-value pairs from genQueryOut */
	char *oldValStr, newValStr[21];		/* for parsing key-value pairs from genQueryOut */
	rodsLong_t newVal;			/* for parsing key-value pairs from genQueryOut */
	sqlResult_t *sqlResult;			/* for parsing key-value pairs from genQueryOut */
	int i;					/* for parsing key-value pairs from genQueryOut */


	RE_TEST_MACRO ("    Calling msiGetCollectionContentsReport")
	
	if (rei == NULL || rei->rsComm == NULL) {
		rodsLog (LOG_ERROR, "msiGetCollectionContentsReport: input rei or rsComm is NULL.");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}


	/* parse inpParam1: our target collection */
	rei->status = parseMspForCollInp (inpParam1, &collInpCache, &outCollInp, 0);
	
	if (rei->status < 0) {
		rodsLog (LOG_ERROR, "msiGetCollectionContentsReport: input inpParam1 error. status = %d", rei->status);
		return (rei->status);
	}


	/* allocate memory for our result struct */
	contents = (keyValPair_t*)malloc(sizeof(keyValPair_t));
	memset (contents, 0, sizeof (keyValPair_t));


	/* Wanted fields. We use coll_id to do a join query on r_data_main and r_coll_main */
	memset (&genQueryInp, 0, sizeof (genQueryInp));
	addInxIval (&genQueryInp.selectInp, COL_DATA_TYPE_NAME, 1);
	addInxIval (&genQueryInp.selectInp, COL_D_DATA_ID, 1);
	addInxIval (&genQueryInp.selectInp, COL_COLL_ID, 1);


	/* Make condition for getting all objects under a collection */
	genAllInCollQCond (outCollInp->collName, collQCond);
	addInxVal (&genQueryInp.sqlCondInp, COL_COLL_NAME, collQCond);
	genQueryInp.maxRows = MAX_SQL_ROWS;
	/* genQueryInp.options = RETURN_TOTAL_ROW_COUNT; */


	/* Query */
	rei->status  = rsGenQuery (rei->rsComm, &genQueryInp, &genQueryOut);


	/* Parse results */
	if (rei->status == 0) {

		/* for each row */
		for (i=0;i<genQueryOut->rowCnt;i++) {
	
			/* get COL_DATA_TYPE_NAME result */
			sqlResult = getSqlResultByInx (genQueryOut, COL_DATA_TYPE_NAME);

			/* retrieve value for this row */
			resultStringToken = sqlResult->value + i*sqlResult->len;

			/* have we found this data type before? */
			oldValStr = getValByKey (contents, resultStringToken);
			if (oldValStr) {
				newVal = atoll(oldValStr) + 1;
			}
			else {
				newVal = 1;
			}
		
			/* add data type name along with its total number of occurrences */
			snprintf(newValStr, 21, "%lld", newVal);
			addKeyVal(contents, resultStringToken, newValStr);
		}

		/* not done? */
		while (rei->status==0 && genQueryOut->continueInx > 0) {
			genQueryInp.continueInx=genQueryOut->continueInx;
			rei->status = rsGenQuery(rei->rsComm, &genQueryInp, &genQueryOut);

			/* for each row */
			for (i=0;i<genQueryOut->rowCnt;i++) {
		
				/* get COL_DATA_TYPE_NAME result */
				sqlResult = getSqlResultByInx (genQueryOut, COL_DATA_TYPE_NAME);
	
				/* retrieve value for this row */
				resultStringToken = sqlResult->value + i*sqlResult->len;
	
				/* have we found this data type before? */
				oldValStr = getValByKey (contents, resultStringToken);
				if (oldValStr) {
					newVal = atoll(oldValStr) + 1;
				}
				else {
					newVal = 1;
				}
			
				/* add data type name along with its total number of occurrences */
				snprintf(newValStr, 21, "%lld", newVal);
				addKeyVal(contents, resultStringToken, newValStr);
			}
		}
	}


	/* send results out to inpParam2 */
	fillMsParam (inpParam2, NULL, KeyValPair_MS_T, contents, NULL);


	/* Return operation status through outParam */
	fillIntInMsParam (outParam, rei->status);
	/* fillIntInMsParam (outParam, genQueryOut->totalRowCount); */

	return (rei->status);
}



/**
 * \fn msiGetCollectionSize(msParam_t *collPath, msParam_t *outKVPairs, msParam_t *status, ruleExecInfo_t *rei)
 *
 * \brief Returns the object count and total disk usage of a collection
 *
 * \module ERA
 *
 * \since pre-2.1
 *
 * \author  Antoine de Torcy
 * \date    2008-10-31
 *
 * \note This microservice returns the object count and total disk usage for all objects in a collection, recursively.
 *    The results are written to a KeyValPair_MS_T whose keyword strings are "Size" and "Object Count".
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] collPath - A CollInp_MS_T or a STR_MS_T with the irods path of the target collection.
 * \param[out] outKVPairs - A KeyValPair_MS_T containing the results.
 * \param[out] status - an INT_MS_T containing the status.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence None
 * \DolVarModified None
 * \iCatAttrDependence None
 * \iCatAttrModified None
 * \sideeffect None
 *
 * \return integer
 * \retval 0 on success
 * \pre None
 * \post None
 * \sa None
**/
int
msiGetCollectionSize(msParam_t *collPath, msParam_t *outKVPairs, msParam_t *status, ruleExecInfo_t *rei)
{
	collInp_t collInpCache, *outCollInp;	/* for parsing collection input param */

	keyValPair_t *res;			/* for passing out results */

	char collQCond[2*MAX_NAME_LEN];		/* for condition in rsGenquery() */
	genQueryInp_t genQueryInp;		/* for query inputs */
	genQueryOut_t *genQueryOut;		/* for query results */

	rodsLong_t size, objCount;		/* to store total size and file count */
	rodsLong_t coll_id;				/* collection ID */
	char tmpStr[21];			/* to store total size and file count */
	
	sqlResult_t *sqlResult;			/* for parsing key-value pairs from genQueryOut */


	RE_TEST_MACRO ("    Calling msiGetCollectionSize")
	
	if (rei == NULL || rei->rsComm == NULL) {
		rodsLog (LOG_ERROR, "msiGetCollectionSize: input rei or rsComm is NULL.");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}


	/* parse inpParam1: our target collection */
	rei->status = parseMspForCollInp (collPath, &collInpCache, &outCollInp, 0);
	
	if (rei->status < 0) {
		rodsLog (LOG_ERROR, "msiGetCollectionSize: input collPath error. status = %d", rei->status);
		return (rei->status);
	}


	/* allocate memory for our result struct */
	res = (keyValPair_t*)malloc(sizeof(keyValPair_t));
	memset (res, 0, sizeof (keyValPair_t));


	/* Wanted fields. */
	memset (&genQueryInp, 0, sizeof (genQueryInp));
	addInxIval (&genQueryInp.selectInp, COL_DATA_SIZE, SELECT_SUM);
	addInxIval (&genQueryInp.selectInp, COL_D_DATA_ID, SELECT_COUNT);
	addInxIval (&genQueryInp.selectInp, COL_DATA_REPL_NUM, 1);


	/* Make condition for getting all objects under a collection */
	genAllInCollQCond (outCollInp->collName, collQCond);
	addInxVal (&genQueryInp.sqlCondInp, COL_COLL_NAME, collQCond);
	genQueryInp.maxRows = MAX_SQL_ROWS;


	/* Query */
	rei->status  = rsGenQuery (rei->rsComm, &genQueryInp, &genQueryOut);


	/* Zero counters, in case no rows found */
	size = 0;
	objCount = 0;


	/* Parse results. Easy here since we have only one "row". */
	if (rei->status == 0) {
	
		/* get COL_DATA_SIZE result */
		sqlResult = getSqlResultByInx (genQueryOut, COL_DATA_SIZE);
		size = atoll(sqlResult->value);
		
		/* get COL_D_DATA_ID result */
		sqlResult = getSqlResultByInx (genQueryOut, COL_D_DATA_ID);
		objCount = atoll(sqlResult->value);

	}


	/* If no rows found, check for empty collection vs invalid path */
	if (rei->status == CAT_NO_ROWS_FOUND)
	{
		/* Call isColl()*/
		rei->status = isColl (rei->rsComm, outCollInp->collName, &coll_id);
	    if (rei->status == CAT_NO_ROWS_FOUND)
	    {
	    	rodsLog (LOG_ERROR, "msiGetCollectionSize: Collection %s was not found. Status = %d",
	    			outCollInp->collName, rei->status);
			free( res ); // JMC cppcheck - leak
	    	return (rei->status);
	    }
	}

	/* store results in keyValPair_t*/
	snprintf(tmpStr, 21, "%lld", size);
	addKeyVal(res, "Size", tmpStr);

	snprintf(tmpStr, 21, "%lld", objCount);
	addKeyVal(res, "Object Count", tmpStr);


	/* send results out to outKVPairs */
	fillMsParam (outKVPairs, NULL, KeyValPair_MS_T, res, NULL);


	/* Return operation status through outParam */
	fillIntInMsParam (status, rei->status);

	return (rei->status);
}


/**
 * \fn msiStructFileBundle(msParam_t *collection, msParam_t *bundleObj, msParam_t *resource, msParam_t *status, ruleExecInfo_t *rei)
 *
 * \brief Bundles a collection for export
 *
 * \module ERA
 *
 * \since 2.1
 *
 * \author  Antoine de Torcy
 * \date    2009-04-21
 *
 * \note This microservice creates a bundle for export from an iRODS collection on a target resource.
 *    Files in the collection are first replicated onto the target resource. If no resource
 *    is given the default resource will be used.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] collection - A CollInp_MS_T or a STR_MS_T with the irods path of the collection to bundle.
 * \param[in] bundleObj - a DataObjInp_MS_T or a STR_MS_T with the bundle object's path.
 * \param[in] resource - Optional - a STR_MS_T which specifies the target resource.
 * \param[out] status - an INT_MS_T containing the operation status.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence None
 * \DolVarModified None
 * \iCatAttrDependence None
 * \iCatAttrModified None
 * \sideeffect None
 *
 * \return integer
 * \retval 0 on success
 * \pre None
 * \post None
 * \sa None
**/
int
msiStructFileBundle(msParam_t *collection, msParam_t *bundleObj, msParam_t *resource, msParam_t *status, ruleExecInfo_t *rei)
{
	collInp_t collInpCache, *collInp;				/* for parsing collection input param */
	dataObjInp_t destObjInpCache, *destObjInp;		/* for parsing bundle object inp. param */
	structFileExtAndRegInp_t *structFileBundleInp; 	/* input for rsStructfileBundle */
	
	
	
	/* For testing mode when used with irule --test */
	RE_TEST_MACRO ("    Calling msiStructfileBundle")
	
	
	/* Sanity test */
	if (rei == NULL || rei->rsComm == NULL) {
			rodsLog (LOG_ERROR, "msistructFileBundle: input rei or rsComm is NULL.");
			return (SYS_INTERNAL_NULL_INPUT_ERR);
	}
	
	
	/* Parse collection input */
	rei->status = parseMspForCollInp (collection, &collInpCache, &collInp, 0);
	
	if (rei->status < 0) {
		rodsLog (LOG_ERROR, "msiStructFileBundle: input collection error. status = %d", rei->status);
		return (rei->status);
	}


	/* Get path of destination bundle object */
	rei->status = parseMspForDataObjInp (bundleObj, &destObjInpCache, &destObjInp, 0);
	
	if (rei->status < 0)
	{
		rodsLog (LOG_ERROR, "msiStructFileBundle: input bundleObj error. status = %d", rei->status);
		return (rei->status);
	}

	
	/* Parse resource input */
	rei->status = parseMspForCondInp (resource, &collInp->condInput, DEST_RESC_NAME_KW);
      
    if (rei->status < 0)  {
        rodsLog (LOG_ERROR, "msistructFileBundle: input resource error. status = %d", rei->status);
        return (rei->status);
    }
    
    
    /* Replicate collection to target resource */
    rei->status = rsCollRepl (rei->rsComm, collInp, NULL);
    
    
    /* Set up input for rsStructFileBundle */
    structFileBundleInp = (structFileExtAndRegInp_t *) malloc (sizeof(structFileExtAndRegInp_t));
    memset (structFileBundleInp, 0, sizeof (structFileExtAndRegInp_t));
    rstrcpy (structFileBundleInp->objPath, destObjInp->objPath, MAX_NAME_LEN);
    rstrcpy (structFileBundleInp->collection, collInp->collName, MAX_NAME_LEN);
    
    /* Add resource info to structFileBundleInp */
    replKeyVal (&collInp->condInput, &structFileBundleInp->condInput);
    
    /* Set data type of target object to tar file (required by rsStructFileBundle) */
    addKeyVal (&structFileBundleInp->condInput, DATA_TYPE_KW, "tar file");
    
    
    /* Invoke rsStructFileBundle and hope for the best */
    rei->status = rsStructFileBundle (rei->rsComm, structFileBundleInp);


	/* Return operation status */
	fillIntInMsParam (status, rei->status);


	return 0;
}


/**
 * \fn msiCollectionSpider(msParam_t *collection, msParam_t *objects, msParam_t *action, msParam_t *status, ruleExecInfo_t *rei)
 *
 * \brief   Applies a microservice sequence to all data objects in a collection, recursively.
 *
 * \module ERA
 *
 * \since 2.1
 *
 * \author  Antoine de Torcy
 * \date    2009-05-09
 *
 * \note This microservice crawls an iRODS collection recursively, and executes a sequence of
 *    microservices/actions for each data object. The data object can be passed as a
 *    DataObjInp_MS_T to this microservice sequence, through 'objects'.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] collection - A CollInp_MS_T or a STR_MS_T with the iRODS path
 * \param[in] objects - Added for clarity. Only the label is required here.
 * \param[in] action - A STR_MS_T (for now) with the microservice sequence. Would need a new type.
 * \param[out] status - An INT_MS_T containing the operation status.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence None
 * \DolVarModified None
 * \iCatAttrDependence None
 * \iCatAttrModified None
 * \sideeffect None
 *
 * \return integer
 * \retval 0 on success
 * \pre None
 * \post None
 * \sa None
**/
int
msiCollectionSpider(msParam_t *collection, msParam_t *objects, msParam_t *action, msParam_t *status, ruleExecInfo_t *rei)
{
	char *actionStr;						/* will contain the microservice sequence */
	collInp_t collInpCache, *collInp;		/* input for rsOpenCollection */
	collEnt_t *collEnt;						/* input for rsReadCollection */
	int handleInx;							/* collection handler */
	msParam_t *msParam;						/* temporary pointer for parameter substitution */
	dataObjInp_t *dataObjInp;				/* will contain pathnames for each object (one at a time) */

	
	
	/* For testing mode when used with irule --test */
	RE_TEST_MACRO ("    Calling msiCollectionSpider")
	

	/* Sanity test */
	if (rei == NULL || rei->rsComm == NULL) {
			rodsLog (LOG_ERROR, "msiCollectionSpider: input rei or rsComm is NULL.");
			return (SYS_INTERNAL_NULL_INPUT_ERR);
	}
	

	/* Parse collection input */
	rei->status = parseMspForCollInp (collection, &collInpCache, &collInp, 0);
	if (rei->status < 0) {
		rodsLog (LOG_ERROR, "msiIsCollectionSpider: input collection error. status = %d", rei->status);
		return (rei->status);
	}
	
	
	/* Check if "objects" input has proper label */
	if (!objects->label || !strlen(objects->label))
	{
		rei->status = USER_PARAM_LABEL_ERR;
		rodsLog (LOG_ERROR, "msiIsCollectionSpider: input objects error. status = %d", rei->status);
		return (rei->status);
	}
	
	
	/* Parse action input */
	if ((actionStr = parseMspForStr(action)) == NULL)
    {
    	rei->status = USER_PARAM_TYPE_ERR;
    	rodsLog (LOG_ERROR, "msiIsCollectionSpider: input action error. status = %d", rei->status);
		return (rei->status);
    }
    
    
    /* Allocate memory for dataObjInp. Needs to be persistent since will be freed later along with other msParams */
    dataObjInp = (dataObjInp_t *)malloc(sizeof(dataObjInp_t));


	/* In our array of msParams, fill the one whose label is the same as 'objects' with a pointer to *dataObjInp */
	msParam = getMsParamByLabel(rei->msParamArray, objects->label);
	if(msParam == NULL)
	{
		addMsParam(rei->msParamArray, strdup(objects->label),
				strdup(DataObjInp_MS_T), (void *)dataObjInp, NULL);
	} else
	{
		resetMsParam (msParam);
		msParam->type = strdup(DataObjInp_MS_T);
		msParam->inOutStruct = (void *)dataObjInp;
	}



	/* Open collection in recursive mode */
	collInp->flags = RECUR_QUERY_FG;
	handleInx = rsOpenCollection (rei->rsComm, collInp);
	if (handleInx < 0)
	{
		rodsLog (LOG_ERROR, "msiCollectionSpider: rsOpenCollection of %s error. status = %d", collInp->collName, handleInx);
		free(actionStr);
		return (handleInx);
	}
	
		
	/* Read our collection one object at a time */
	while ( ((rei->status = rsReadCollection (rei->rsComm, &handleInx, &collEnt)) >= 0) && NULL != collEnt )  // JMC cppcheck - nullptr
	{
		
		/* Skip collection entries */
		if (collEnt->objType == DATA_OBJ_T)
		{
			/* Write our current object's path in dataObjInp, where the inOutStruct in 'objects' points to */
			memset(dataObjInp, 0, sizeof(dataObjInp_t));
			snprintf(dataObjInp->objPath, MAX_NAME_LEN, "%s/%s", collEnt->collName, collEnt->dataName);
		
			/* Run actionStr on our object */
			rei->status = applyRule(actionStr, rei->msParamArray, rei, 0);
			if (rei->status < 0)
			{
				/* If an error occurs, log incident but keep going */			
				rodsLog (LOG_ERROR, "msiCollectionSpider: execMyRule error. status = %d", rei->status);
			}
		
		}

		
		/* Free collEnt only. Content will be freed by rsCloseCollection() */
		//if (collEnt) // JMC cppcheck - redundant nullptr test
		//{
			free(collEnt);	    
		//}
	}
	
			
	/* Close collection */
	rei->status = rsCloseCollection (rei->rsComm, &handleInx);


	/* Return operation status */
	fillIntInMsParam (status, rei->status);
	

	/* Done */
	return rei->status;
}



