/**
 * @file reSysDataObjOpr.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* reSysDataObjOpr.c */

#include "reSysDataObjOpr.hpp"
#include "genQuery.hpp"
#include "getRescQuota.hpp"
#include "reServerLib.hpp"
#include "dataObjOpr.hpp"
#include "resource.hpp"
#include "physPath.hpp"

// =-=-=-=-=-=-=-
//
#include "irods_resource_backport.hpp"
#include "irods_server_api_table.hpp"

/**
 * \fn msiSetDefaultResc (msParam_t *xdefaultRescList, msParam_t *xoptionStr, ruleExecInfo_t *rei)
 *
 * \brief  This microservice sets the default resource and query resource metadata for
 *    the subsequent use based on an input array and condition given
 *    in the dataObject Input Structure.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Mike Wan
 * \date    2006-11
 *
 * \note This function is mandatory even no defaultResc is specified (null) and should be executed right after the screening function msiSetNoDirectRescInp.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] xdefaultRescList - Required - a msParam of type STR_MS_T which is a list
 *    of %-delimited resourceNames. It is a resource to use if no resource is input.
 *    A "null" means there is no defaultResc.
 * \param[in] xoptionStr - a msParam of type STR_MS_T which is an option (preferred, forced, random)
 *    with random as default. A "forced" input means the defaultResc will be used regardless
 *    of the user input. The forced action only apply to to users with normal privilege.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence rei->doinp->condInput,
 *                    rei->rsComm->proxyUser.authInfo.authFlag
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 **/
int
msiSetDefaultResc( msParam_t *xdefaultRescList, msParam_t *xoptionStr, ruleExecInfo_t *rei ) {
    char *defaultRescList;
    char *optionStr;
    std::string default_resc;

    defaultRescList = ( char * ) xdefaultRescList->inOutStruct;

    optionStr = ( char * ) xoptionStr->inOutStruct;

    RE_TEST_MACRO( "    Calling msiSetDefaultResc" )

    // JMC - legacy resource - rei->status = setDefaultResc (rei->rsComm, defaultRescList, optionStr, &rei->doinp->condInput, &myRescGrpInfo );

    irods::error err = irods::set_default_resource( rei->rsComm, defaultRescList, optionStr, &rei->doinp->condInput, default_resc );
    rei->status = err.code();

    if ( rei->status >= 0 ) {
        snprintf( rei->rescName, NAME_LEN, "%s", default_resc.c_str() );
    }
    else {
        irods::log( PASS( err ) );
        memset( &rei->rescName, 0, NAME_LEN );
    }
    return rei->status;
}

/**
 * \fn msiSetRescSortScheme (msParam_t *xsortScheme, ruleExecInfo_t *rei)
 *
 * \brief  This microservice sets the scheme for selecting the best resource to use when creating a data object.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Mike Wan
 * \date 2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] xsortScheme - The sorting scheme. Valid schemes are "default", "random" and
 *    "byRescType". The "byRescType" scheme will put the cache class of resource on the top
 *    of the list. The scheme "random" and "byRescType" can be applied in sequence.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 **/
int
msiSetRescSortScheme( msParam_t*, ruleExecInfo_t* ) {
    rodsLog( LOG_ERROR, "msiSetRescSortScheme is no longer supported, please use a resource composition of a similar type" );
    return SYS_NOT_SUPPORTED;
}


/**
 * \fn msiSetNoDirectRescInp (msParam_t *xrescList, ruleExecInfo_t *rei)
 *
 * \brief  This microservice sets a list of resources that cannot be used by a normal
 *  user directly.  It checks a given list of taboo-resources against the
 *  user provided resource name and disallows if the resource is in the list of taboo-resources.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Mike Wan
 * \date    2006-11
 *
 * \note This microservice is optional, but if used, should be the first function to execute because it screens the resource input.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] xrescList - InpParam is a xrescList of type STR_MS_T which is a list of %-delimited resourceNames e.g., resc1%resc2%resc3.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence rei->doinp->condInput - user set  resource list
 *                   rei->rsComm->proxyUser.authInfo.authFlag
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 if user set resource is allowed or user is privileged.
 * \retval USER_DIRECT_RESC_INPUT_ERR  if resource is taboo.
 * \pre none
 * \post none
 * \sa none
 **/
int
msiSetNoDirectRescInp( msParam_t *xrescList, ruleExecInfo_t *rei ) {

    keyValPair_t *condInput;
    char *rescName;
    char *value;
    strArray_t strArray;
    int status, i;
    char *rescList;

    rescList = ( char * ) xrescList->inOutStruct;

    RE_TEST_MACRO( "    Calling msiSetNoDirectRescInp" )

    rei->status = 0;

    if ( rescList == NULL || strcmp( rescList, "null" ) == 0 ) {
        return 0;
    }

    if ( rei->rsComm->proxyUser.authInfo.authFlag >= LOCAL_PRIV_USER_AUTH ) {
        /* have enough privilege */
        return 0;
    }

    condInput = &rei->doinp->condInput;

    if ( ( rescName = getValByKey( condInput, BACKUP_RESC_NAME_KW ) ) == NULL &&
            ( rescName = getValByKey( condInput, DEST_RESC_NAME_KW ) ) == NULL &&
            ( rescName = getValByKey( condInput, DEF_RESC_NAME_KW ) ) == NULL &&
            ( rescName = getValByKey( condInput, RESC_NAME_KW ) ) == NULL ) {
        return 0;
    }

    memset( &strArray, 0, sizeof( strArray ) );

    status = parseMultiStr( rescList, &strArray );

    if ( status <= 0 ) {
        return 0;
    }

    value = strArray.value;
    for ( i = 0; i < strArray.len; i++ ) {
        if ( strcmp( rescName, &value[i * strArray.size] ) == 0 ) {
            /* a match */
            rei->status = USER_DIRECT_RESC_INPUT_ERR;
            free( value );
            return USER_DIRECT_RESC_INPUT_ERR;
        }
    }
    if ( value != NULL ) {
        free( value );
    }
    return 0;
}

/**
 * \fn msiSetDataObjPreferredResc (msParam_t *xpreferredRescList, ruleExecInfo_t *rei)
 *
 * \brief  If the data has multiple copies, this microservice specifies the preferred copy to use.
 * It sets the preferred resources of the opened object.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Mike Wan
 * \date 2007
 *
 * \note The copy stored in this preferred resource will be picked if it exists. More than
 * one resource can be input using the character "%" as separator.
 * e.g., resc1%resc2%resc3. The most preferred resource should be at the top of the list.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] xpreferredRescList - a msParam of type STR_MS_T, comma-delimited list of resources
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence - rei->doinp->openFlags, rei->doi
 * \DolVarModified - rei->doi
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 **/
int
msiSetDataObjPreferredResc( msParam_t *xpreferredRescList, ruleExecInfo_t *rei ) {
    int writeFlag;
    char *value;
    strArray_t strArray;
    int status, i;
    char *preferredRescList;

    preferredRescList = ( char * ) xpreferredRescList->inOutStruct;

    RE_TEST_MACRO( "    Calling msiSetDataObjPreferredResc" )

    rei->status = 0;

    if ( preferredRescList == NULL || strcmp( preferredRescList, "null" ) == 0 ) {
        return 0;
    }

    writeFlag = getWriteFlag( rei->doinp->openFlags );

    memset( &strArray, 0, sizeof( strArray ) );

    status = parseMultiStr( preferredRescList, &strArray );

    if ( status <= 0 ) {
        return 0;
    }

    if ( rei->doi == NULL || rei->doi->next == NULL ) {
        return 0;
    }

    value = strArray.value;
    for ( i = 0; i < strArray.len; i++ ) {
        if ( requeDataObjInfoByResc( &rei->doi, &value[i * strArray.size],
                                     writeFlag, 1 ) >= 0 ) {
            rei->status = 1;
            return rei->status;
        }
    }
    return rei->status;
}

/**
 * \fn msiSetDataObjAvoidResc (msParam_t *xavoidResc, ruleExecInfo_t *rei)
 *
 * \brief  This microservice specifies the copy to avoid.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Mike Wan
 * \date 2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] xavoidResc - a msParam of type STR_MS_T - the name of the resource to avoid
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence - rei->doinp->openFlags, rei->doi
 * \DolVarModified - rei->doi
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 **/
int
msiSetDataObjAvoidResc( msParam_t *xavoidResc, ruleExecInfo_t *rei ) {
    int writeFlag;
    char *avoidResc;

    avoidResc = ( char * ) xavoidResc->inOutStruct;

    RE_TEST_MACRO( "    Calling msiSetDataObjAvoidResc" )

    rei->status = 0;

    writeFlag = getWriteFlag( rei->doinp->openFlags );

    if ( avoidResc != NULL && strcmp( avoidResc, "null" ) != 0 ) {
        if ( requeDataObjInfoByResc( &rei->doi, avoidResc, writeFlag, 0 )
                >= 0 ) {
            rei->status = 1;
        }
    }
    return rei->status;
}

/**
 * \fn msiSortDataObj (msParam_t *xsortScheme, ruleExecInfo_t *rei)
 *
 * \brief  This microservice sorts the copies of the data object using this scheme. Currently, "random" sorting scheme is supported.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Mike Wan
 * \date 2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] xsortScheme - input sorting scheme.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence - rei->doi
 * \DolVarModified - rei->doi
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 **/
int
msiSortDataObj( msParam_t *xsortScheme, ruleExecInfo_t *rei ) {
    char *sortScheme;

    sortScheme = ( char * ) xsortScheme->inOutStruct;
    RE_TEST_MACRO( "    Calling msiSortDataObj" )

    rei->status = 0;
    if ( sortScheme != NULL ) {
        if ( strcmp( sortScheme, "random" ) == 0 ) {
            sortDataObjInfoRandom( &rei->doi );
            // JMC - legacy resource -     } else if (strcmp (sortScheme, "byRescClass") == 0) {
            //    rei->status = sortObjInfoForOpen( &rei->doi, NULL, 1 );
        }
    }
    return rei->status;
}


/**
 * \fn msiSysChksumDataObj (ruleExecInfo_t *rei)
 *
 * \brief  This microservice performs a checksum on the uploaded or copied data object.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Mike Wan
 * \date 2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence - rei->doi, rei->rsComm
 * \DolVarModified - rei->doi->chksum (the entire link list of doi)
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 **/
int
msiSysChksumDataObj( ruleExecInfo_t *rei ) {
    dataObjInfo_t *dataObjInfoHead;
    char *chksumStr = NULL;

    RE_TEST_MACRO( "    Calling msiSysChksumDataObj" )

    rei->status = 0;

    /* don't cache replicate or copy operation */

    dataObjInfoHead = rei->doi;

    if ( dataObjInfoHead == NULL ) {
        return 0;
    }

    if ( strlen( dataObjInfoHead->chksum ) == 0 ) {
        /* not already checksumed */
        rei->status = dataObjChksumAndReg( rei->rsComm, dataObjInfoHead,
                                           &chksumStr );
        if ( chksumStr != NULL ) {
            rstrcpy( dataObjInfoHead->chksum, chksumStr, NAME_LEN );
            free( chksumStr );
        }
    }

    return 0;
}

/**
 * \fn msiSetDataTypeFromExt (ruleExecInfo_t *rei)
 *
 * \brief This microservice checks if the filename has an extension
 *    (string following a period (.)) and if so, checks if the iCAT has a matching
 *    entry for it, and if so sets the dataObj data type.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Wayne Schroeder
 * \date   2007-02-09
 *
 * \note  Always returns success since it is only doing an attempt;
 *   that is, failure is common and not really a failure.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 **/
int
msiSetDataTypeFromExt( ruleExecInfo_t *rei ) {
    int status;
    RE_TEST_MACRO( "    Calling msiSetDataType" )

    rei->status = 0;

    if ( rei->doi == NULL ) {
        return 0;
    }

    dataObjInfo_t *dataObjInfoHead = rei->doi;

    char logicalCollName[MAX_NAME_LEN];
    char logicalFileName[MAX_NAME_LEN] = "";
    status = splitPathByKey( dataObjInfoHead->objPath,
                             logicalCollName, sizeof( logicalCollName ), logicalFileName, sizeof( logicalFileName ), '/' );
    if ( strlen( logicalFileName ) <= 0 ) {
        return 0;
    }

    char logicalFileNameNoExtension[MAX_NAME_LEN] = "";
    char logicalFileNameExt[MAX_NAME_LEN] = "";
    status = splitPathByKey( logicalFileName,
                             logicalFileNameNoExtension, sizeof( logicalFileNameNoExtension ), logicalFileNameExt, sizeof( logicalFileNameExt ), '.' );
    if ( strlen( logicalFileNameExt ) <= 0 ) {
        return 0;
    }

    /* see if there's an entry in the catalog for this extension */
    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    addInxIval( &genQueryInp.selectInp, COL_TOKEN_NAME, 1 );

    char condStr1[MAX_NAME_LEN];
    snprintf( condStr1, sizeof( condStr1 ), "= 'data_type'" );
    addInxVal( &genQueryInp.sqlCondInp,  COL_TOKEN_NAMESPACE, condStr1 );

    char condStr2[MAX_NAME_LEN];
    snprintf( condStr2, sizeof( condStr2 ), "like '%s|.%s|%s'", "%", logicalFileNameExt, "%" );
    addInxVal( &genQueryInp.sqlCondInp,  COL_TOKEN_VALUE2, condStr2 );

    genQueryInp.maxRows = 1;

    genQueryOut_t *genQueryOut = NULL;
    status = rsGenQuery( rei->rsComm, &genQueryInp, &genQueryOut );
    if ( status != 0 || genQueryOut == NULL ) {
        return 0;
    }

    rodsLog( LOG_DEBUG, "msiSetDataTypeFromExt: query status %d rowCnt=%d", status, genQueryOut->rowCnt );

    if ( genQueryOut->rowCnt != 1 ) {
        return 0;
    }

    status = svrCloseQueryOut( rei->rsComm, genQueryOut );

    /* register it */
    keyValPair_t regParam;
    memset( &regParam, 0, sizeof( regParam ) );
    addKeyVal( &regParam, DATA_TYPE_KW,  genQueryOut->sqlResult[0].value );

    if ( strcmp(dataObjInfoHead->rescHier, "") ) {
        addKeyVal( &regParam, IN_PDMO_KW, dataObjInfoHead->rescHier );
    }

    modDataObjMeta_t modDataObjMetaInp;
    modDataObjMetaInp.dataObjInfo = dataObjInfoHead;
    modDataObjMetaInp.regParam = &regParam;

    status = rsModDataObjMeta( rei->rsComm, &modDataObjMetaInp );

    return 0;
}

/**
 * \fn msiStageDataObj (msParam_t *xcacheResc, ruleExecInfo_t *rei)
 *
 * \brief  This microservice stages the data object to the specified resource
 *    before operation. It stages a copy of the data object in the cacheResc before
 *    opening the data object.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Mike Wan
 * \date 2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] xcacheResc - The resource name in which to cache the object
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence - rei->doi, rei->doinp->oprType, rei->doinp->openFlags.
 * \DolVarModified - rei->doi (new replica queued on top)
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 **/
int
msiStageDataObj( msParam_t *xcacheResc, ruleExecInfo_t *rei ) {
    int status;
    char *cacheResc;

    cacheResc = ( char * ) xcacheResc->inOutStruct;

    RE_TEST_MACRO( "    Calling msiStageDataObj" )

    rei->status = 0;

    if ( cacheResc == NULL || strcmp( cacheResc, "null" ) == 0 ) {
        return rei->status;
    }

    /* preProcessing */
    if ( rei->doinp->oprType == REPLICATE_OPR ||
            rei->doinp->oprType == COPY_DEST ||
            rei->doinp->oprType == COPY_SRC ) {
        return rei->status;
    }

    if ( getValByKey( &rei->doinp->condInput, RESC_NAME_KW ) != NULL ||
            getValByKey( &rei->doinp->condInput, REPL_NUM_KW ) != NULL ) {
        /* a specific replNum or resource is specified. Don't cache */
        return rei->status;
    }

    status = msiSysReplDataObj( xcacheResc, NULL, rei );

    return status;
}

/**
 * \fn msiSysReplDataObj (msParam_t *xcacheResc, msParam_t *xflag, ruleExecInfo_t *rei)
 *
 * \brief  This microservice replicates a data object. It can be used to replicate
 *  a copy of the file just uploaded or copied data object to the specified
 *  replResc.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Mike Wan
 * \date 2007
 *
 * \note The allFlag is only meaningful if the replResc is a resource group.
 *  In this case, setting allFlag to "all" means a copy will be made in all the
 *  resources in the resource group. A "null" input means a single copy will be made in
 *  one of the resources in the resource group.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 *
 * \param[in] xcacheResc -
 * \param[in] xflag -
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence - rei->doi, rei->doinp->openFlags
 * \DolVarModified - rei->doi (new replica queued on top)
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 **/
int
msiSysReplDataObj( msParam_t *xcacheResc, msParam_t *xflag,
                   ruleExecInfo_t *rei ) {
//    int writeFlag;
    dataObjInfo_t *dataObjInfoHead;
    char *cacheResc;
    char *flag = NULL;

    cacheResc = ( char * ) xcacheResc->inOutStruct;
    if ( xflag != NULL && xflag->inOutStruct != NULL ) {
        flag = ( char * ) xflag->inOutStruct;
    }

    RE_TEST_MACRO( "    Calling msiSysReplDataObj" )

    rei->status = 0;

    if ( cacheResc == NULL || strcmp( cacheResc, "null" ) == 0 ||
            strlen( cacheResc ) == 0 ) {
        return rei->status;
    }

    dataObjInfoHead = rei->doi;

    if ( dataObjInfoHead == NULL ) {
        return rei->status;
    }

//    writeFlag = getWriteFlag( rei->doinp->openFlags );

//    if ( requeDataObjInfoByResc( &dataObjInfoHead, cacheResc, writeFlag, 1 )
//            >= 0 ) {
//        /* we have a good copy on cache */
//        rei->status = 1;
//        return ( rei->status );
//    }

    rei->status = rsReplAndRequeDataObjInfo( rei->rsComm, &dataObjInfoHead,
                  cacheResc, flag );
    if ( rei->status >= 0 ) {
        rei->doi = dataObjInfoHead;
    }
    return rei->status;
}

/**
 * \fn msiSetNumThreads (msParam_t *xsizePerThrInMbStr, msParam_t *xmaxNumThrStr, msParam_t *xwindowSizeStr, ruleExecInfo_t *rei)
 *
 * \brief  This microservice specifies the parameters for determining the number of
 *    threads to use for data transfer. It sets the number of threads and the TCP window size.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Mike Wan
 * \date 2007
 *
 * \note The msiSetNumThreads function must be present or no thread will be used for all transfer.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] xsizePerThrInMbStr - The number of threads is computed
 *    using: numThreads = fileSizeInMb / sizePerThrInMb + 1 where sizePerThrInMb
 *    is an integer value in MBytes. It also accepts the word "default" which sets
 *    sizePerThrInMb to a default value of 32.
 * \param[in] xmaxNumThrStr - The maximum number of threads to use. It accepts integer
 *    value up to 64. It also accepts the word "default" which sets maxNumThr to a default value of 4.
 * \param[in] xwindowSizeStr - The TCP window size in Bytes for the parallel transfer. A value of 0 or "default" means a default size of 1,048,576 Bytes.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence - rei->doinp->numThreads, rei->doinp->dataSize
 * \DolVarModified - rei->rsComm->windowSize (rei->rsComm == NULL, OK),
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 **/
int
msiSetNumThreads( msParam_t *xsizePerThrInMbStr, msParam_t *xmaxNumThrStr,
                  msParam_t *xwindowSizeStr, ruleExecInfo_t *rei ) {
    int sizePerThr;
    int maxNumThr;
    dataObjInp_t *doinp;
    int numThr;
    char *sizePerThrInMbStr;
    char *maxNumThrStr;
    char *windowSizeStr;

    sizePerThrInMbStr = ( char * ) xsizePerThrInMbStr->inOutStruct;
    maxNumThrStr = ( char * ) xmaxNumThrStr->inOutStruct;
    windowSizeStr = ( char * ) xwindowSizeStr->inOutStruct;

    if ( rei->rsComm != NULL ) {
        if ( strcmp( windowSizeStr, "null" ) == 0 ||
                strcmp( windowSizeStr, "default" ) == 0 ) {
            rei->rsComm->windowSize = 0;
        }
        else {
            rei->rsComm->windowSize = atoi( windowSizeStr );
        }
    }

    if ( strcmp( sizePerThrInMbStr, "default" ) == 0 ) {
        sizePerThr = SZ_PER_TRAN_THR;
    }
    else {
        sizePerThr = atoi( sizePerThrInMbStr ) * ( 1024 * 1024 );
        if ( sizePerThr <= 0 ) {
            rodsLog( LOG_ERROR,
                     "msiSetNumThreads: Bad input sizePerThrInMb %s", sizePerThrInMbStr );
            sizePerThr = SZ_PER_TRAN_THR;
        }
    }

    doinp = rei->doinp;
    if ( doinp == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiSetNumThreads: doinp is NULL" );
        rei->status = DEF_NUM_TRAN_THR;
        return DEF_NUM_TRAN_THR;
    }

    if ( strcmp( maxNumThrStr, "default" ) == 0 ) {
        maxNumThr = DEF_NUM_TRAN_THR;
    }
    else {
        maxNumThr = atoi( maxNumThrStr );
        if ( maxNumThr < 0 ) {
            rodsLog( LOG_ERROR,
                     "msiSetNumThreads: Bad input maxNumThr %s", maxNumThrStr );
            maxNumThr = DEF_NUM_TRAN_THR;
        }
        else if ( maxNumThr == 0 ) {
            rei->status = 0;
            return rei->status;
        }
        else if ( maxNumThr > MAX_NUM_CONFIG_TRAN_THR ) {
            rodsLog( LOG_ERROR,
                     "msiSetNumThreads: input maxNumThr %s too large", maxNumThrStr );
            maxNumThr = MAX_NUM_CONFIG_TRAN_THR;
        }
    }


    if ( doinp->numThreads > 0 ) {
        numThr = doinp->dataSize / TRANS_BUF_SZ + 1;
        if ( numThr > doinp->numThreads ) {
            numThr = doinp->numThreads;
        }
    }
    else {
        numThr = doinp->dataSize / sizePerThr + 1;
    }

    if ( numThr > maxNumThr ) {
        numThr = maxNumThr;
    }

    rei->status = numThr;
    return rei->status;

}

/**
 * \fn msiDeleteDisallowed (ruleExecInfo_t *rei)
 *
 * \brief  This microservice sets the policy for determining that certain data cannot be deleted.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Mike Wan
 * \date 2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 **/
int
msiDeleteDisallowed( ruleExecInfo_t *rei ) {
    RE_TEST_MACRO( "    Calling msiDeleteDisallowed" )

    rei->status = SYS_DELETE_DISALLOWED;

    return rei->status;
}

/**
 * \fn msiOprDisallowed (ruleExecInfo_t *rei)
 *
 * \brief  This generic microservice sets the policy for determining that certain operation is not allowed.
 *
 * \module core
 *
 * \since 2.3
 *
 * \author
 * \date
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence
 * \DolVarModified
 * \iCatAttrDependence
 * \iCatAttrModified
 * \sideeffect
 *
 * \return integer
 * \retval 0 on success
 * \pre
 * \post
 * \sa
 **/
int
msiOprDisallowed( ruleExecInfo_t *rei ) {
    RE_TEST_MACRO( "    Calling msiOprDisallowed" )

    rei->status = MSI_OPERATION_NOT_ALLOWED;

    return rei->status;
}


/**
 * \fn msiSetMultiReplPerResc (ruleExecInfo_t *rei)
 *
 * \brief  By default, the system allows one copy per resource. This microservice sets the number of copies per resource to unlimited.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Mike Wan
 * \date 2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified - rei->statusStr
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 **/
int
msiSetMultiReplPerResc( ruleExecInfo_t *rei ) {
    rstrcpy( rei->statusStr, MULTI_COPIES_PER_RESC, MAX_NAME_LEN );
    return 0;
}

/**
 * \fn msiNoChkFilePathPerm (ruleExecInfo_t *rei)
 *
 * \brief  This microservice does not check file path permissions when registering a file. This microservice is REPLACED by msiSetChkFilePathPerm
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Mike Wan
 * \date 2007
 *
 * \warning This microservice can create a security problem if used incorrectly.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval NO_CHK_PATH_PERM
 * \pre none
 * \post none
 * \sa none
 **/
int
msiNoChkFilePathPerm( ruleExecInfo_t *rei ) {
    rei->status = NO_CHK_PATH_PERM;
    return NO_CHK_PATH_PERM;
}
/**
 * \fn msiSetChkFilePathPerm (msParam_t *xchkType, ruleExecInfo_t *rei)
 *
 * \brief  This microservice set the check type for file path permission check when registering a file.
 *
 * \module core
 *
 * \since pre-3.1
 *
 * \author Mike Wan
 * \date 2012
 *
 * \warning This microservice can create a security problem if set to anything other than DISALLOW_PATH_REG and used incorrectly.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] - xchkType - Required - a msParam of type STR_MS_T which defines the check type to set. Valid values are DO_CHK_PATH_PERM_STR, NO_CHK_PATH_PERM_STR, CHK_NON_VAULT_PATH_PERM_STR and DISALLOW_PATH_REG_STR.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval DO_CHK_PATH_PERM, NO_CHK_PATH_PERM, CHK_NON_VAULT_PATH_PERM or DISALLOW_PATH_REG.
 * \pre none
 * \post none
 * \sa none
 **/
int
msiSetChkFilePathPerm( msParam_t *xchkType, ruleExecInfo_t *rei ) {
    char *chkType;

    chkType = ( char * ) xchkType->inOutStruct;

    if ( strcmp( chkType, DO_CHK_PATH_PERM_STR ) == 0 ) {
        rei->status = DO_CHK_PATH_PERM;
    }
    else if ( strcmp( chkType, NO_CHK_PATH_PERM_STR ) == 0 ) {
        rei->status = NO_CHK_PATH_PERM;
    }
    else if ( strcmp( chkType, CHK_NON_VAULT_PATH_PERM_STR ) == 0 ) {
        rei->status = CHK_NON_VAULT_PATH_PERM;
    }
    else if ( strcmp( chkType, DISALLOW_PATH_REG_STR ) == 0 ) {
        rei->status = DISALLOW_PATH_REG;
    }
    else {
        rodsLog( LOG_ERROR,
                 "msiNoChkFilePathPerm:invalid check type %s,set to DISALLOW_PATH_REG" );
        rei->status = DISALLOW_PATH_REG;
    }
    return rei->status;
}
// =-=-=-=-=-=-=-

/**
 * \fn msiNoTrashCan (ruleExecInfo_t *rei)
 *
 * \brief  This microservice sets the policy to no trash can.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Mike Wan
 * \date 2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval NO_TRASH_CAN
 * \pre none
 * \post none
 * \sa none
 **/
int
msiNoTrashCan( ruleExecInfo_t *rei ) {
    rei->status = NO_TRASH_CAN;
    return NO_TRASH_CAN;
}

/**
 * \fn msiSetPublicUserOpr (msParam_t *xoprList, ruleExecInfo_t *rei)
 *
 * \brief  This microservice sets a list of operations that can be performed
 * by the user "public".
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Mike Wan
 * \date 2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] xoprList - Only 2 operations are allowed - "read" - read files;
 * "query" - browse some system level metadata. More than one operation can be
 * input using the character "%" as separator. e.g., read%query.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence - rei->rsComm->clientUser.authInfo.authFlag
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 **/
int
msiSetPublicUserOpr( msParam_t *xoprList, ruleExecInfo_t *rei ) {

    char *value;
    strArray_t strArray;
    int status, i;
    char *oprList;

    oprList = ( char * ) xoprList->inOutStruct;

    RE_TEST_MACRO( "    Calling msiSetPublicUserOpr" )

    rei->status = 0;

    if ( oprList == NULL || strcmp( oprList, "null" ) == 0 ) {
        return 0;
    }

    if ( rei->rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        /* not enough privilege */
        return SYS_NO_API_PRIV;
    }

    memset( &strArray, 0, sizeof( strArray ) );

    status = parseMultiStr( oprList, &strArray );

    if ( status <= 0 ) {
        return 0;
    }

    value = strArray.value;
    for ( i = 0; i < strArray.len; i++ ) {
        if ( strcmp( "read", &value[i * strArray.size] ) == 0 ) {
            /* a match */
            setApiPerm( DATA_OBJ_OPEN_AN, PUBLIC_USER_AUTH, PUBLIC_USER_AUTH );
            setApiPerm( FILE_OPEN_AN, REMOTE_PRIV_USER_AUTH,
                        PUBLIC_USER_AUTH );
            setApiPerm( FILE_READ_AN, REMOTE_PRIV_USER_AUTH,
                        PUBLIC_USER_AUTH );
            setApiPerm( DATA_OBJ_LSEEK_AN, PUBLIC_USER_AUTH,
                        PUBLIC_USER_AUTH );
            setApiPerm( FILE_LSEEK_AN, REMOTE_PRIV_USER_AUTH,
                        PUBLIC_USER_AUTH );
            setApiPerm( DATA_OBJ_CLOSE_AN, PUBLIC_USER_AUTH,
                        PUBLIC_USER_AUTH );
            setApiPerm( FILE_CLOSE_AN, REMOTE_PRIV_USER_AUTH,
                        PUBLIC_USER_AUTH );
            setApiPerm( OBJ_STAT_AN, PUBLIC_USER_AUTH,
                        PUBLIC_USER_AUTH );
            setApiPerm( DATA_OBJ_GET_AN, PUBLIC_USER_AUTH, PUBLIC_USER_AUTH );
            setApiPerm( DATA_GET_AN, REMOTE_PRIV_USER_AUTH, PUBLIC_USER_AUTH );
        }
        else if ( strcmp( "query", &value[i * strArray.size] ) == 0 ) {
            setApiPerm( GEN_QUERY_AN, PUBLIC_USER_AUTH, PUBLIC_USER_AUTH );
        }
        else {
            rodsLog( LOG_ERROR,
                     "msiSetPublicUserOpr: operation %s for user public not allowed",
                     &value[i * strArray.size] );
        }
    }

    if ( value != NULL ) {
        free( value );
    }

    return 0;
}

int
setApiPerm( int apiNumber, int proxyPerm, int clientPerm ) {
    int apiInx;

    if ( proxyPerm < NO_USER_AUTH || proxyPerm > LOCAL_PRIV_USER_AUTH ) {
        rodsLog( LOG_ERROR,
                 "setApiPerm: input proxyPerm %d out of range", proxyPerm );
        return SYS_INPUT_PERM_OUT_OF_RANGE;
    }

    if ( clientPerm < NO_USER_AUTH || clientPerm > LOCAL_PRIV_USER_AUTH ) {
        rodsLog( LOG_ERROR,
                 "setApiPerm: input clientPerm %d out of range", clientPerm );
        return SYS_INPUT_PERM_OUT_OF_RANGE;
    }

    apiInx = apiTableLookup( apiNumber );

    if ( apiInx < 0 ) {
        return apiInx;
    }

    irods::api_entry_table& RsApiTable = irods::get_server_api_table();
    RsApiTable[apiInx]->proxyUserAuth = proxyPerm;
    RsApiTable[apiInx]->clientUserAuth = clientPerm;

    return 0;
}

/**
 * \fn msiSetGraftPathScheme (msParam_t *xaddUserName, msParam_t *xtrimDirCnt, ruleExecInfo_t *rei)
 *
 * \brief  This microservice sets the VaultPath scheme to GRAFT_PATH.
 *    It grafts (adds) the logical path to the vault path of the resource
 *    when generating the physical path for a data object.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Mike Wan
 * \date 2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] xaddUserName - This msParam specifies whether the userName should
 *      be added to the physical path. e.g. $vaultPath/$userName/$logicalPath.
 *      "xaddUserName" can have two values - yes or no.
 * \param[in] xtrimDirCnt - This msParam specifies the number of leading directory
 *      elements of the logical path to trim. Sometimes it may not be desirable to
 *      graft the entire logical path. e.g.,for a logicalPath /myZone/home/me/foo/bar,
 *      it may be desirable to graft just the part "foo/bar" to the vaultPath.
 *      "xtrimDirCnt" should be set to 3 in this case.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence - rei->inOutMsParamArray (label == VAULT_PATH_POLICY)
 * \DolVarModified - rei->inOutMsParamArray (label == VAULT_PATH_POLICY)
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 **/
int
msiSetGraftPathScheme( msParam_t *xaddUserName, msParam_t *xtrimDirCnt,
                       ruleExecInfo_t *rei ) {
    char *addUserNameStr;
    char *trimDirCntStr;
    int addUserName;
    int trimDirCnt;
    msParam_t *msParam;
    vaultPathPolicy_t *vaultPathPolicy;

    RE_TEST_MACRO( "    Calling msiSetGraftPathScheme" )

    addUserNameStr = ( char * ) xaddUserName->inOutStruct;
    trimDirCntStr = ( char * ) xtrimDirCnt->inOutStruct;

    if ( strcmp( addUserNameStr, "no" ) == 0 ) {
        addUserName = 0;
    }
    else if ( strcmp( addUserNameStr, "yes" ) == 0 ) {
        addUserName = 1;
    }
    else {
        rodsLog( LOG_ERROR,
                 "msiSetGraftPathScheme: invalid input addUserName %s", addUserNameStr );
        rei->status = SYS_INPUT_PERM_OUT_OF_RANGE;
        return SYS_INPUT_PERM_OUT_OF_RANGE;
    }

    if ( !isdigit( trimDirCntStr[0] ) ) {
        rodsLog( LOG_ERROR,
                 "msiSetGraftPathScheme: input trimDirCnt %s", trimDirCntStr );
        rei->status = SYS_INPUT_PERM_OUT_OF_RANGE;
        return SYS_INPUT_PERM_OUT_OF_RANGE;
    }
    else {
        trimDirCnt = atoi( trimDirCntStr );
    }

    rei->status = 0;

    if ( ( msParam = getMsParamByLabel( &rei->inOutMsParamArray,
                                        VAULT_PATH_POLICY ) ) != NULL ) {
        vaultPathPolicy = ( vaultPathPolicy_t * ) msParam->inOutStruct;
        if ( vaultPathPolicy == NULL ) {
            vaultPathPolicy = ( vaultPathPolicy_t* )malloc( sizeof( vaultPathPolicy_t ) );
            msParam->inOutStruct = ( void * ) vaultPathPolicy;
        }
        vaultPathPolicy->scheme = GRAFT_PATH_S;
        vaultPathPolicy->addUserName = addUserName;
        vaultPathPolicy->trimDirCnt = trimDirCnt;
        return 0;
    }
    else {
        vaultPathPolicy = ( vaultPathPolicy_t * ) malloc(
                              sizeof( vaultPathPolicy_t ) );
        vaultPathPolicy->scheme = GRAFT_PATH_S;
        vaultPathPolicy->addUserName = addUserName;
        vaultPathPolicy->trimDirCnt = trimDirCnt;
        addMsParam( &rei->inOutMsParamArray, VAULT_PATH_POLICY,
                    VaultPathPolicy_MS_T, ( void * ) vaultPathPolicy, NULL );
    }
    return 0;
}

/**
 * \fn msiSetRandomScheme (ruleExecInfo_t *rei)
 *
 * \brief  This microservice sets the scheme for composing the physical path in the vault to RANDOM.  A randomly generated path is appended to the
 *         vaultPath when generating the physical path. e.g., $vaultPath/$userName/$randomPath. The advantage with the RANDOM scheme is renaming
 *         operations (imv, irm) are much faster because there is no need to rename the corresponding physical path.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author - Mike Wan
 * \date - 2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence - rei->inOutMsParamArray (label==VAULT_PATH_POLICY)
 * \DolVarModified - rei->inOutMsParamArray (label==VAULT_PATH_POLICY)
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 **/
int
msiSetRandomScheme( ruleExecInfo_t *rei ) {
    msParam_t *msParam;
    vaultPathPolicy_t *vaultPathPolicy;

    RE_TEST_MACRO( "    Calling msiSetRandomScheme" )

    rei->status = 0;

    if ( ( msParam = getMsParamByLabel( &rei->inOutMsParamArray,
                                        VAULT_PATH_POLICY ) ) != NULL ) {
        vaultPathPolicy = ( vaultPathPolicy_t * ) msParam->inOutStruct;
        if ( vaultPathPolicy == NULL ) {
            vaultPathPolicy = ( vaultPathPolicy_t* )malloc( sizeof( vaultPathPolicy_t ) );
            msParam->inOutStruct = ( void * ) vaultPathPolicy;
        }
        memset( vaultPathPolicy, 0, sizeof( vaultPathPolicy_t ) );
        vaultPathPolicy->scheme = RANDOM_S;
        return 0;
    }
    else {
        vaultPathPolicy = ( vaultPathPolicy_t * ) malloc(
                              sizeof( vaultPathPolicy_t ) );
        memset( vaultPathPolicy, 0, sizeof( vaultPathPolicy_t ) );
        vaultPathPolicy->scheme = RANDOM_S;
        addMsParam( &rei->inOutMsParamArray, VAULT_PATH_POLICY,
                    VaultPathPolicy_MS_T, ( void * ) vaultPathPolicy, NULL );
    }
    return 0;
}


/**
 * \fn msiSetReServerNumProc (msParam_t *xnumProc, ruleExecInfo_t *rei)
 *
 * \brief  Sets number of processes for the rule engine server
 *
 * \module core
 *
 * \since 2.1
 *
 * \author Mike Wan
 * \date 2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] xnumProc - a STR_MS_T representing number of processes
 *     - this value can be "default" or an integer
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 **/
int
msiSetReServerNumProc( msParam_t *xnumProc, ruleExecInfo_t *rei ) {
    char *numProcStr;
    int numProc;

    numProcStr = ( char* )xnumProc->inOutStruct;

    if ( strcmp( numProcStr, "default" ) == 0 ) {
        numProc = DEF_NUM_RE_PROCS;
    }
    else {
        numProc = atoi( numProcStr );
        if ( numProc > MAX_RE_PROCS ) {
            numProc = MAX_RE_PROCS;
        }
        else if ( numProc < 0 ) {
            numProc = DEF_NUM_RE_PROCS;
        }
    }
    rei->status = numProc;

    return numProc;
}

/**
 * \fn msiSetRescQuotaPolicy (msParam_t *xflag, ruleExecInfo_t *rei)
 *
 * \brief  This microservice sets the resource quota to on or off.
 *
 * \module core
 *
 * \since 2.3
 *
 * \author  Mike Wan
 * \date    2010-02
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] xflag - Required - a msParam of type STR_MS_T.
 *     \li "on" - enable Resource Quota enforcement
 *     \li "off" - disable Resource Quota enforcement (default)
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence None,
 * \DolVarModified RescQuotaPolicy.
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval RESC_QUOTA_OFF or RESC_QUOTA_ON.
 * \pre none
 * \post none
 * \sa none
 **/
int
msiSetRescQuotaPolicy( msParam_t *xflag, ruleExecInfo_t *rei ) {
    char *flag;

    flag = ( char * ) xflag->inOutStruct;

    RE_TEST_MACRO( "    Calling msiSetRescQuotaPolic" )

    if ( strcmp( flag, "on" ) == 0 ) {
        rei->status = RescQuotaPolicy = RESC_QUOTA_ON;
    }
    else {
        rei->status = RescQuotaPolicy = RESC_QUOTA_OFF;
    }
    return rei->status;
}

/**
 * \fn msiSetReplComment (msParam_t *inpParam1, msParam_t *inpParam2,
 *        msParam_t *inpParam3, msParam_t *inpParam4, ruleExecInfo_t *rei)
 *
 * \brief This microservice sets the data_comments attribute of a data object.
 *
 * \module core
 *
 * \since 2.4
 *
 * \author  Thomas Ledoux (integrated by Wayne Schroeder)
 * \date    2010-04-30
 *
 * \note  Can be called by client through irule
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - a STR_MS_T with the id of the object (can be null if unknown, the next param will then be used)
 * \param[in] inpParam2 - a msParam of type DataObjInp_MS_T or a STR_MS_T which would be taken as dataObj path
 * \param[in] inpParam3 - a INT which gives the replica number
 * \param[in] inpParam4 - a STR_MS_T containing the comment
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 **/
int
msiSetReplComment( msParam_t *inpParam1, msParam_t *inpParam2,
                   msParam_t *inpParam3, msParam_t *inpParam4,
                   ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    char *dataCommentStr, *dataIdStr;    /* for parsing of input params */

    modDataObjMeta_t modDataObjMetaInp;  /* for rsModDataObjMeta() */
    dataObjInfo_t dataObjInfo;           /* for rsModDataObjMeta() */
    keyValPair_t regParam;                       /* for rsModDataObjMeta() */

    RE_TEST_MACRO( "    Calling msiSetReplComment" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR, "msiSetReplComment: input rei or rsComm is NULL." );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
    rsComm = rei->rsComm ;

    memset( &dataObjInfo, 0, sizeof( dataObjInfo ) );

    /* parse inpParam1: data object ID */
    if ( ( dataIdStr = parseMspForStr( inpParam1 ) ) != NULL )  {
        dataObjInfo.dataId = ( rodsLong_t ) atoll( dataIdStr );
    }
    else {
        dataObjInfo.dataId = 0;
    }

    /* parse inpParam2: data object path */
    if ( parseMspForStr( inpParam2 ) ) {
        snprintf( dataObjInfo.objPath, sizeof( dataObjInfo.objPath ), "%s", parseMspForStr( inpParam2 ) );
    }
    /* make sure to have at least data ID or path */
    if ( !( dataIdStr || strlen( dataObjInfo.objPath ) > 0 ) ) {
        rodsLog( LOG_ERROR, "msiSetReplComment: No data object ID or path provided." );
        return USER__NULL_INPUT_ERR;
    }

    if ( inpParam3 != NULL ) {
        dataObjInfo.replNum = parseMspForPosInt( inpParam3 );
    }

    /* parse inpParam3: data type string */
    if ( ( dataCommentStr = parseMspForStr( inpParam4 ) ) == NULL ) {
        rodsLog( LOG_ERROR, "msiSetReplComment: parseMspForStr error for param 4." );
        return USER__NULL_INPUT_ERR;
    }
    memset( &regParam, 0, sizeof( regParam ) );
    addKeyVal( &regParam, DATA_COMMENTS_KW, dataCommentStr );
    if ( strcmp(dataObjInfo.rescHier, "") ) {
        addKeyVal( &regParam, IN_PDMO_KW, dataObjInfo.rescHier );
    }

    rodsLog( LOG_NOTICE, "msiSetReplComment: mod %s (%d) with %s",
             dataObjInfo.objPath, dataObjInfo.replNum, dataCommentStr );
    /* call rsModDataObjMeta() */
    modDataObjMetaInp.dataObjInfo = &dataObjInfo;
    modDataObjMetaInp.regParam = &regParam;
    rei->status = rsModDataObjMeta( rsComm, &modDataObjMetaInp );

    /* failure? */
    if ( rei->status < 0 ) {
        if ( strlen( dataObjInfo.objPath ) > 0 ) {
            rodsLog( LOG_ERROR,
                     "msiSetReplComment: rsModDataObjMeta failed for object %s, status = %d",
                     dataObjInfo.objPath, rei->status );
        }
        else {
            rodsLog( LOG_ERROR,
                     "msiSetReplComment: rsModDataObjMeta failed for object ID %d, status = %d",
                     dataObjInfo.dataId, rei->status );
        }
    }
    else {
        rodsLog( LOG_NOTICE, "msiSetReplComment: OK mod %s (%d) with %s",
                 dataObjInfo.objPath, dataObjInfo.replNum, dataCommentStr );
    }
    return rei->status;
}

/**
 * \fn msiSetBulkPutPostProcPolicy (msParam_t *xflag, ruleExecInfo_t *rei)
 *
 * \brief  This microservice sets whether the post processing "put"
 *  rule (acPostProcForPut) should be run (on or off) for the bulk put
 *  operation.
 *
 * \module core
 *
 * \since 2.4
 *
 * \author  Mike Wan
 * \date    2010-07
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] xflag - Required - a msParam of type STR_MS_T.
 *     \li "on" - enable execution of acPostProcForPut.
 *     \li "off" - disable execution of acPostProcForPut.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval POLICY_OFF or POLICY_ON
 * \pre none
 * \post none
 * \sa none
 **/
int
msiSetBulkPutPostProcPolicy( msParam_t *xflag, ruleExecInfo_t *rei ) {
    char *flag;

    flag = ( char * ) xflag->inOutStruct;

    RE_TEST_MACRO( "    Calling msiSetBulkPutPostProcPolicy" )

    if ( strcmp( flag, "on" ) == 0 ) {
        rei->status = POLICY_ON;
    }
    else {
        rei->status = POLICY_OFF;
    }
    return rei->status;
}

/**
 * \fn msiSysMetaModify (msParam_t *sysMetadata, msParam_t *value, ruleExecInfo_t *rei)
 *
 * \brief Modify system metadata.
 *
 * \module core
 *
 * \since after 2.4.1
 *
 * \author  Jean-Yves Nief
 * \date    2011-01-05
 *
 * \note  This call should only be used through the rcExecMyRule (irule) call
 *        i.e., rule execution initiated by clients and should not be called
 *        internally by the server since it interacts with the client through
 *        the normal client/server socket connection.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] sysMetadata - A STR_MS_T which specifies the system metadata to be modified.
 *            Allowed values are: "datatype", "comment", "expirytime". If one wants to modify
 *                       only the sys metadata for one given replica, the value should be for example // JMC - backport 4573
 *                   "comment++++numRepl=2". It will only modify the comment for the replica number 2.
 *            If the syntax after "++++" is invalid, it will be ignored and all replica will be modified.
 *            This does not work for the "datatype" metadata.
 * \param[in] value - A STR_MS_T which specifies the value to be given to the system metadata.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 upon success
 * \pre N/A
 * \post N/A
 * \sa N/A
 **/
int
msiSysMetaModify( msParam_t *sysMetadata, msParam_t *value, ruleExecInfo_t *rei ) {
    keyValPair_t regParam;
    modDataObjMeta_t modDataObjMetaInp;
    // =-=-=-=-=-=-=-
    // JMC - backport 4573
    dataObjInfo_t dataObjInfo;
    char theTime[TIME_LEN], *inpStr = 0, mdname[MAX_NAME_LEN], replAttr[MAX_NAME_LEN], *pstr1 = 0, *pstr2 = 0;
    int allRepl = 0, len1 = 0, len2 = 0, numRepl = 0, status = 0;
    // =-=-=-=-=-=-=-
    rsComm_t *rsComm = 0;

    RE_TEST_MACRO( " Calling msiSysMetaModify" )

    memset( &mdname, 0, sizeof( mdname ) );


    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiSysMetaModify: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    if ( sysMetadata == NULL ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiSysMetaModify: input Param1 is NULL" );
        rei->status = USER__NULL_INPUT_ERR;
        return rei->status;
    }

    if ( value == NULL ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiSysMetaModify: input Param2 is NULL" );
        rei->status = USER__NULL_INPUT_ERR;
        return rei->status;
    }

    if ( strcmp( sysMetadata->type, STR_MS_T ) == 0 && strcmp( value->type, STR_MS_T ) == 0 ) {
        memset( &regParam, 0, sizeof( regParam ) );
        // =-=-=-=-=-=-=-
        // JMC - backport 4573
        memcpy( &dataObjInfo, rei->doi, sizeof( dataObjInfo_t ) );
        inpStr = ( char * ) sysMetadata->inOutStruct;
        allRepl = 1;
        /* parse the input parameter which is: <string> or <string>++++replNum=<int> */
        pstr1 = strstr( inpStr, "++++" );
        if ( pstr1 != NULL ) {
            len1 = strlen( inpStr ) - strlen( pstr1 );
            if ( len1 > 0 ) {
                strncpy( mdname, inpStr, len1 );
            }
            pstr2 = strstr( pstr1 + 4, "=" );
            if ( pstr2 != NULL ) {
                len2 = strlen( pstr1 + 4 ) - strlen( pstr2 );
                memset( replAttr, 0, sizeof( replAttr ) ); // JMC - backport 4803
                strncpy( replAttr, pstr1 + 4, len2 );
                if ( len2 > 0 ) {
                    if ( strcmp( replAttr, "numRepl" ) == 0 ) {
                        numRepl = atoi( pstr2 + 1 );
                        if ( ( numRepl == 0 && strcmp( pstr2 + 1, "0" ) == 0 ) || numRepl > 0 ) {
                            dataObjInfo.replNum = numRepl;
                            allRepl = 0;
                        }
                    }
                }
            }
        }
        else {
            strncpy( mdname , inpStr, strlen( inpStr ) );
            allRepl = 1;
        }
        /* end of the parsing */
        // =-=-=-=-=-=-=-
        if ( strcmp( mdname, "datatype" ) == 0 ) {
            addKeyVal( &regParam, DATA_TYPE_KW, ( char * ) value->inOutStruct );
        }
        else if ( strcmp( mdname, "comment" ) == 0 ) {
            addKeyVal( &regParam, DATA_COMMENTS_KW, ( char * ) value->inOutStruct );
            // =-=-=-=-=-=-=-
            // JMC - backport 4573
            if ( allRepl == 1 ) {
                addKeyVal( &regParam, ALL_KW, ( char * ) value->inOutStruct );
            }

            // =-=-=-=-=-=-=-
        }
        else if ( strcmp( mdname, "expirytime" ) == 0 ) {
            rstrcpy( theTime, ( char * ) value->inOutStruct, TIME_LEN );
            if ( strncmp( theTime, "+", 1 ) == 0 ) {
                rstrcpy( theTime, ( char * ) value->inOutStruct + 1, TIME_LEN ); /* skip the + */
                status = checkDateFormat( theTime );    /* check and convert the time value */
                getOffsetTimeStr( theTime, theTime );   /* convert delta format to now + this*/
            }
            else {
                status = checkDateFormat( theTime );    /* check and convert the time value */
            }
            if ( status != 0 )  {
                rei->status = DATE_FORMAT_ERR;
                rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                    "msiSysMetaModify: bad format for the input time: %s. Please refer to isysmeta help.",
                                    ( char * ) value->inOutStruct );
                return rei->status;
            }
            else {
                addKeyVal( &regParam, DATA_EXPIRY_KW, theTime );
                // =-=-=-=-=-=-=-
                // JMC - backport 4573
                if ( allRepl == 1 ) {
                    addKeyVal( &regParam, ALL_KW, theTime );
                }
                // =-=-=-=-=-=-=-
            }
        }
        else {
            rei->status = USER_BAD_KEYWORD_ERR;
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiSysMetaModify: unknown system metadata or impossible to modify it: %s",
                                ( char * ) sysMetadata->inOutStruct );
        }

        if ( strcmp(dataObjInfo.rescHier,  "") ) {
            addKeyVal( &regParam, IN_PDMO_KW, dataObjInfo.rescHier );
        }

        modDataObjMetaInp.dataObjInfo = &dataObjInfo; // JMC - backport 4573
        modDataObjMetaInp.regParam = &regParam;
        rei->status = rsModDataObjMeta( rsComm, &modDataObjMetaInp );
    }
    else {       /* one or two bad input parameter type for the msi */
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiSysMetaModify: Unsupported input Param1 type %s or Param2 type %s",
                            sysMetadata->type, value->type );
        rei->status = UNKNOWN_PARAM_IN_RULE_ERR;
        return rei->status;
    }

    return rei->status;
}
