/**
 * @file  reDataObjOpr.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "reDataObjOpr.hpp"
#include "apiHeaderAll.hpp"
#include "rsApiHandler.hpp"
#include "collection.hpp"

#include <string>
#include <vector>
#include <boost/regex.hpp>
#include <boost/algorithm/string/regex.hpp>

/**
 * \fn msiDataObjCreate (msParam_t *inpParam1, msParam_t *msKeyValStr,
 *        msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief Creates a data object in the iCAT.
 *
 * \module core
 *
 * \since 2.1
 *
 * \author  Mike Wan
 * \date    2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - A DataObjInp_MS_T or STR_MS_T which would be taken as dataObj path.
 * \param[in] msKeyValStr - Optional - a STR_MS_T. This is the special msKeyValStr
 *   format of keyWd1=value1++++keyWd2=value2++++keyWd3=value3...
 *   If the keyWd is not specified (without the '=' char), the value is
 *   assumed to be the target resource ("destRescName") for backward
 *   compatibility.
 *   Valid keyWds are:
 *    \li "destRescName" - the target resource.
 *    \li "forceFlag" - overwrite existing copy. This keyWd has
 *        no value. But the '=' character is still needed
 *    \li "createMode" - the file mode of the data object.
 *    \li "dataType" - the data type of the data object.
 *    \li "dataSize" - the size of the data object. This input is optional.
 * \param[out] outParam - a INT_MS_T containing the descriptor of the create.
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
msiDataObjCreate( msParam_t *inpParam1, msParam_t *msKeyValStr,
                  msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    dataObjInp_t dataObjInp, *myDataObjInp;
    char *outBadKeyWd;
    int validKwFlags;

    RE_TEST_MACRO( "    Calling msiDataObjCreate" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjCreate: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* parse inpParam1 */
    rei->status = parseMspForDataObjInp( inpParam1, &dataObjInp,
                                         &myDataObjInp, 0 );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjCreate: input inpParam1 error. status = %d", rei->status );
        return rei->status;
    }

    validKwFlags = DEST_RESC_NAME_FLAG | CREATE_MODE_FLAG | DATA_TYPE_FLAG |
                   FORCE_FLAG_FLAG | DATA_SIZE_FLAGS | OBJ_PATH_FLAG;
    rei->status = parseMsKeyValStrForDataObjInp( msKeyValStr, myDataObjInp,
                  DEST_RESC_NAME_KW, validKwFlags, &outBadKeyWd );

    if ( rei->status < 0 ) {
        if ( outBadKeyWd != NULL ) {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjCreate: input keyWd - %s error. status = %d",
                                outBadKeyWd, rei->status );
            free( outBadKeyWd );
        }
        else {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjCreate: input msKeyValStr error. status = %d",
                                rei->status );
        }
        return rei->status;
    }

    rei->status = rsDataObjCreate( rsComm, myDataObjInp );

    if ( myDataObjInp == &dataObjInp ) {
        clearKeyVal( &myDataObjInp->condInput );
    }

    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjCreate: rsDataObjCreate failed for %s, status = %d",
                            myDataObjInp->objPath,
                            rei->status );
    }

    return rei->status;
}

/**
 * \fn msiDataObjOpen (msParam_t *inpParam, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief  This a microservice performs a low-level open for existing data object
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Mike Wan
 * \date    2007
 *
 * \note  Can be called by client through irule
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam - a msParam of type DataObjInp_MS_T or a STR_MS_T which would be taken as msKeyValStr.
 *  msKeyValStr -  This is the special msKeyValStr
 *    format of keyWd1=value1++++keyWd2=value2++++keyWd3=value3...
 *    If the keyWd is not specified (without the '=' char), the value is
 *    assumed to be the path of the data object("objPath") for backward
 *    compatibility.
 *    	Valid keyWds are:
 *          \li "objPath" - the path of the data object to open.
 *          \li "rescName" - the resource of the data object to open.
 *          \li  "replNum" - the replica number of the copy to open.
 *          \li "openFlags" - the open flags. valid open flags are:
 *            O_RDONLY, O_WRONLY, O_RDWR and O_TRUNC. These
 *            can be combined by concatenation, e.g.
 *            O_WRONLYO_TRUNC (without the '|' character). The
 *            default open flag is O_RDONLY.
 * \param[out] outParam - a msParam of type INT_MS_T containing the descriptor of the open.
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
 * \retval positive on success
 * \pre none
 * \post none
 * \sa none
**/
int
msiDataObjOpen( msParam_t *inpParam, msParam_t *outParam,
                ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    dataObjInp_t dataObjInp, *myDataObjInp;
    char *outBadKeyWd = NULL;
    int validKwFlags;

    RE_TEST_MACRO( "    Calling msiDataObjOpen" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjOpen: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* parse inpParam */
    if ( strcmp( inpParam->type, STR_MS_T ) == 0 ) {
        bzero( &dataObjInp, sizeof( dataObjInp ) );
        myDataObjInp = &dataObjInp;
        validKwFlags = OBJ_PATH_FLAG | RESC_NAME_FLAG | OPEN_FLAGS_FLAG |
                       REPL_NUM_FLAG;
        rei->status = parseMsKeyValStrForDataObjInp( inpParam, myDataObjInp,
                      OBJ_PATH_KW, validKwFlags, &outBadKeyWd );
    }
    else {
        rei->status = parseMspForDataObjInp( inpParam, &dataObjInp,
                                             &myDataObjInp, 0 );
    }

    if ( rei->status < 0 ) {
        if ( outBadKeyWd != NULL ) {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjOpen: input keyWd - %s error. status = %d",
                                outBadKeyWd, rei->status );
            free( outBadKeyWd );
        }
        else {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjOpen: input msKeyValStr error. status = %d",
                                rei->status );
        }
        return rei->status;
    }

    rei->status = rsDataObjOpen( rsComm, myDataObjInp );
    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjOpen: rsDataObjOpen failed for %s, status = %d",
                            dataObjInp.objPath,
                            rei->status );
    }

    return rei->status;
}

/**
 * \fn msiDataObjClose (msParam_t *inpParam, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief  This microservice performs a low-level close for an opened/created data object.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Mike Wan
 * \date    2007
 *
 * \note  Can be called by client through irule
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam - inpParam is a msParam of type INT_MS_T or STR_MS_T.
 * \param[out] outParam - outParam is a msParam of type INT_MS_T.
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
msiDataObjClose( msParam_t *inpParam, msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    openedDataObjInp_t dataObjCloseInp, *myDataObjCloseInp;

    RE_TEST_MACRO( "    Calling msiDataObjClose" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjClose: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    if ( inpParam == NULL ) {
        rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjClose: input inpParam is NULL" );
        return rei->status;
    }

    if ( strcmp( inpParam->type, DataObjCloseInp_MS_T ) == 0 ) {
        myDataObjCloseInp = ( openedDataObjInp_t* )inpParam->inOutStruct;
    }
    else {
        int myInt;

        myInt = parseMspForPosInt( inpParam );
        if ( myInt >= 0 ) {
            memset( &dataObjCloseInp, 0, sizeof( dataObjCloseInp ) );
            dataObjCloseInp.l1descInx = myInt;
            myDataObjCloseInp = &dataObjCloseInp;
        }
        else {
            rei->status = myInt;
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjClose: parseMspForPosInt error for param1." );
            return rei->status;
        }
    }

    rei->status = rsDataObjClose( rsComm, myDataObjCloseInp );
    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjClose: rsDataObjClose failed, status = %d",
                            rei->status );
    }

    return rei->status;
}

/**
 * \fn msiDataObjLseek (msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *inpParam3, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief  This a microservice performs a low-level (file) seek of an opened data object.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Mike Wan
 * \date    2007
 *
 * \note  Can be called by client through irule
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - a msParam of type DataObjLseekInp_MS_T or INT_MS_T or a STR_MS_T which would be the descriptor.
 * \param[in] inpParam2 - Optional - a msParam of type INT_MS_T or DOUBLE_MS_T or a STR_MS_T which would be the offset.
 * \param[in] inpParam3 - Optional - a msParam of type INT_MS_T or a STR_MS_T which would be from where. Can be SEEK_SET, SEEK_CUR, and SEEK_END.
 * \param[out] outParam - a msParam of type Double_MS_T or DataObjLseekOut_MS_T which is the return status.
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
 * \retval positive on success
 * \pre none
 * \post none
 * \sa none
**/
int
msiDataObjLseek( msParam_t *inpParam1, msParam_t *inpParam2,
                 msParam_t *inpParam3, msParam_t *outParam,
                 ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    openedDataObjInp_t dataObjLseekInp, *myDataObjLseekInp;
    fileLseekOut_t *dataObjLseekOut = NULL;

    RE_TEST_MACRO( "    Calling msiDataObjLseek" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjLseek: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    if ( inpParam1 == NULL ) {
        rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjLseek: input inpParam1 is NULL" );
        return rei->status;
    }

    if ( strcmp( inpParam1->type, STR_MS_T ) == 0 ) {
        /* str input */
        memset( &dataObjLseekInp, 0, sizeof( dataObjLseekInp ) );
        dataObjLseekInp.l1descInx = atoi( ( char* )inpParam1->inOutStruct );
        myDataObjLseekInp = &dataObjLseekInp;
    }
    else if ( strcmp( inpParam1->type, INT_MS_T ) == 0 ) {
        memset( &dataObjLseekInp, 0, sizeof( dataObjLseekInp ) );
        dataObjLseekInp.l1descInx = *( int * )inpParam1->inOutStruct;
        myDataObjLseekInp = &dataObjLseekInp;
    }
    else if ( strcmp( inpParam1->type, DataObjLseekInp_MS_T ) == 0 ) {
        myDataObjLseekInp = ( openedDataObjInp_t* )inpParam1->inOutStruct;
    }
    else {
        rei->status = USER_PARAM_TYPE_ERR;
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjLseek: Unsupported input Param type %s",
                            inpParam1->type );
        return rei->status;
    }

    if ( inpParam2 != NULL ) {
        if ( strcmp( inpParam2->type, INT_MS_T ) == 0 ) {
            myDataObjLseekInp->offset = *( int * )inpParam2->inOutStruct;
        }
        else if ( strcmp( inpParam2->type, STR_MS_T ) == 0 ) {
            /* str input */
            if ( strcmp( ( char * ) inpParam2->inOutStruct, "null" ) != 0 ) {
                myDataObjLseekInp->offset = strtoll( ( char* )inpParam2->inOutStruct,
                                                     0, 0 );
            }
        }
        else if ( strcmp( inpParam2->type, DOUBLE_MS_T ) == 0 ) {
            myDataObjLseekInp->offset = *( rodsLong_t * )inpParam2->inOutStruct;
        }
        else {
            rei->status = USER_PARAM_TYPE_ERR;
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjLseek: Unsupported input Param type %s",
                                inpParam2->type );
            return rei->status;
        }
    }

    if ( inpParam3 != NULL ) {
        /* str input */
        if ( strcmp( inpParam3->type, STR_MS_T ) == 0 ) {
            if ( strcmp( ( char * ) inpParam3->inOutStruct, "SEEK_SET" ) == 0 ) {
                myDataObjLseekInp->whence = SEEK_SET;
            }
            else if ( strcmp( ( char * ) inpParam3->inOutStruct,
                              "SEEK_CUR" ) == 0 ) {
                myDataObjLseekInp->whence = SEEK_CUR;
            }
            else if ( strcmp( ( char * ) inpParam3->inOutStruct,
                              "SEEK_END" ) == 0 ) {
                myDataObjLseekInp->whence = SEEK_END;
            }
            else if ( strcmp( ( char * ) inpParam3->inOutStruct, "null" ) != 0 ) {
                myDataObjLseekInp->whence = atoi( ( char* )inpParam3->inOutStruct );
            }
        }
        else if ( strcmp( inpParam3->type, INT_MS_T ) == 0 ) {
            myDataObjLseekInp->whence = *( int * )inpParam3->inOutStruct;
        }
        else {
            rei->status = USER_PARAM_TYPE_ERR;
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjLseek: Unsupported input Param type %s",
                                inpParam3->type );
            return rei->status;
        }
    }

    if ( myDataObjLseekInp->whence != SEEK_SET &&
            myDataObjLseekInp->whence != SEEK_CUR &&
            myDataObjLseekInp->whence != SEEK_END ) {
        rei->status = USER_PARAM_TYPE_ERR;
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjLseek: Unsupported input whence value %d",
                            myDataObjLseekInp->whence );
        return rei->status;
    }

    rei->status = rsDataObjLseek( rsComm, myDataObjLseekInp, &dataObjLseekOut );
    if ( rei->status >= 0 && outParam != NULL ) {
        fillMsParam( outParam, NULL, DataObjLseekOut_MS_T, dataObjLseekOut, NULL );
    }
    else {
        free( dataObjLseekOut );
        if ( rei->status >= 0 ) {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjLseek: rsDataObjLseek failed, status = %d",
                                rei->status );
        }
    }

    return rei->status;
}

/**
 * \fn msiDataObjRead (msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief  This microservice performs a low-level read of an opened data object
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Mike Wan
 * \date    2007
 *
 * \note  Can be called by client through irule
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - a msParam of type DataObjReadInp_MS_T or INT_MS_T or STR_MS_T which would be the descriptor.
 * \param[in] inpParam2 - Optional - a msParam of type INT_MS_T or STR_MS_T which would be the length.
 * \param[out] outParam - a msParam of type BUF_LEN_MS_T.
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
 * \retval positive on success
 * \pre none
 * \post none
 * \sa none
**/
int
msiDataObjRead( msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    openedDataObjInp_t dataObjReadInp, *myDataObjReadInp;
    bytesBuf_t *dataObjReadOutBBuf = NULL;
    int myInt;

    RE_TEST_MACRO( "    Calling msiDataObjRead" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjRead: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    if ( inpParam1 == NULL ) {
        rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjRead: input inpParam1 is NULL" );
        return rei->status;
    }

    if ( strcmp( inpParam1->type, DataObjReadInp_MS_T ) == 0 ) {
        myDataObjReadInp = ( openedDataObjInp_t* )inpParam1->inOutStruct;
    }
    else {
        myInt = parseMspForPosInt( inpParam1 );
        if ( myInt >= 0 ) {
            memset( &dataObjReadInp, 0, sizeof( dataObjReadInp ) );
            dataObjReadInp.l1descInx = myInt;
            myDataObjReadInp = &dataObjReadInp;
        }
        else {
            rei->status = myInt;
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjRead: parseMspForPosInt error for param1." );
            return rei->status;
        }
    }

    if ( inpParam2 != NULL ) {
        myInt = parseMspForPosInt( inpParam2 );

        if ( myInt < 0 ) {
            if ( myInt != SYS_NULL_INPUT ) {
                rei->status = myInt;
                rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                    "msiDataObjRead: parseMspForPosInt error for param2." );
                return rei->status;
            }
        }
        else {
            myDataObjReadInp->len = myInt;
        }
    }

    dataObjReadOutBBuf = ( bytesBuf_t * ) malloc( sizeof( bytesBuf_t ) );
    memset( dataObjReadOutBBuf, 0, sizeof( bytesBuf_t ) );

    rei->status = rsDataObjRead( rsComm, myDataObjReadInp, dataObjReadOutBBuf );

    if ( rei->status >= 0 ) {
        fillBufLenInMsParam( outParam, rei->status, dataObjReadOutBBuf );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjRead: rsDataObjRead failed, status = %d",
                            rei->status );
    }

    return rei->status;
}

/**
 * \fn msiDataObjWrite (msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief  This a microservice performs a low-level write to an opened data object
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Mike Wan
 * \date    2007
 *
 * \note Can be called by client through irule
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - a msParam of type DataObjWriteInp_MS_T or INT_MS_T or a STR_MS_T which would be the descriptor.
 * \param[in] inpParam2 - Optional - a msParam of type BUF_LEN_MS_T or a STR_MS_T, the input is inpOutBuf and the length of the buffer in the BBuf.
 *    \li "stderr", "stdout" can be passed as well
 * \param[out] outParam - a msParam of type INT_MS_T for the length written.
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
 * \retval positive on success
 * \pre none
 * \post none
 * \sa none
**/
int
msiDataObjWrite( msParam_t *inpParam1, msParam_t *inpParam2,
                 msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    openedDataObjInp_t dataObjWriteInp, *myDataObjWriteInp;
    int myInt;
    bytesBuf_t tmpBBuf, *myBBuf = NULL;
    execCmdOut_t *myExecCmdOut;
    msParam_t *mP;

    RE_TEST_MACRO( "    Calling msiDataObjWrite" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR, "msiDataObjWrite: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;


    if ( inpParam1 == NULL || ( inpParam1->inpOutBuf == NULL &&
                                inpParam1->inOutStruct == NULL ) ) {
        rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjWrite: input inpParam1 or inpOutBuf or inOutStruct is NULL" );
        return rei->status;
    }

    if ( strcmp( inpParam1->type, DataObjWriteInp_MS_T ) == 0 ) {
        myDataObjWriteInp = ( openedDataObjInp_t* )inpParam1->inOutStruct;
    }
    else {
        myInt = parseMspForPosInt( inpParam1 );
        if ( myInt >= 0 ) {
            memset( &dataObjWriteInp, 0, sizeof( dataObjWriteInp ) );
            dataObjWriteInp.l1descInx = myInt;
            myDataObjWriteInp = &dataObjWriteInp;
        }
        else {
            rei->status = myInt;
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjWrite: parseMspForPosInt error for param1." );
            return rei->status;
        }
    }

    if ( inpParam2 != NULL ) {
        if ( ( strcmp( ( char * )inpParam2->inOutStruct, "stdout" ) == 0 ) ||
                ( strcmp( ( char * ) inpParam2->inOutStruct, "stderr" ) == 0 ) ) {
            if ( ( mP = getMsParamByLabel( rei->msParamArray, "ruleExecOut" ) ) == NULL ) {
                return NO_VALUES_FOUND;
            }
            myExecCmdOut = ( execCmdOut_t* )mP->inOutStruct;
            if ( strcmp( ( char * ) inpParam2->inOutStruct, "stdout" ) == 0 ) {
                free( inpParam2->inOutStruct );
                inpParam2->inOutStruct = 0;
                if ( myExecCmdOut->stdoutBuf.len > 0 ) {
                    inpParam2->inOutStruct =  strdup( ( char * ) myExecCmdOut->stdoutBuf.buf );
                }
                else {
                    inpParam2->inOutStruct = strdup( "" );
                }
            }
            else {
                free( inpParam2->inOutStruct );
                if ( myExecCmdOut->stdoutBuf.len > 0 ) {
                    inpParam2->inOutStruct =  strdup( ( char * ) myExecCmdOut->stderrBuf.buf );
                }
                else {
                    inpParam2->inOutStruct = strdup( "" );
                }
            }
            inpParam2->type = strdup( STR_MS_T );
        }
        if ( strcmp( inpParam2->type, STR_MS_T ) == 0 ) {
            tmpBBuf.len = myDataObjWriteInp->len =
                              strlen( ( char* )inpParam2->inOutStruct ) + 1;
            tmpBBuf.buf = inpParam2->inOutStruct;
            myBBuf = &tmpBBuf;
        }
        else {
            myInt = parseMspForPosInt( inpParam2 );
            if ( myInt < 0 ) {
                if ( myInt != SYS_NULL_INPUT ) {
                    rei->status = myInt;
                    rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                        "msiDataObjWrite: parseMspForPosInt error for param2." );
                    return rei->status;
                }
            }
            else {
                myDataObjWriteInp->len = myInt;
            }
            myBBuf = inpParam2->inpOutBuf;
        }
        rei->status = rsDataObjWrite( rsComm, myDataObjWriteInp, myBBuf );
    }

    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjWrite: rsDataObjWrite failed, status = %d",
                            rei->status );
    }

    return rei->status;
}

/**
 * \fn msiDataObjUnlink (msParam_t *inpParam, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief  This microservice deletes an existing data object
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author    Mike Wan
 * \date      2007
 *
 * \note  Can be called by client through irule
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam - a msParam of type DataObjInp_MS_T or STR_MS_T which would be taken as msKeyValStr.
 *    msKeyValStr -  This is the special msKeyValStr
 *    format of keyWd1=value1++++keyWd2=value2++++keyWd3=value3...
 *    If the keyWd is not specified (without the '=' char), the value is
 *    assumed to be the path of the data object("objPath") for backward
 *    compatibility.
 *    Valid keyWds are:
 *        \li "objPath" - the path of the data object to remove.
 *        \li "replNum" - the replica number of the copy to remove.
 *        \li "forceFlag" - Remove the data object instead putting
 *            it in trash. This keyWd has no value. But the
 *            '=' character is still needed
 *        \li "irodsAdminRmTrash" - Admin rm trash. This keyWd has no value.
 *        \li "irodsRmTrash" - rm trash. This keyWd has no value.
 *        \li "unreg" - unregister the path. This keyWd has no value.
 *
 * \param[out] outParam - a msParam of type INT_MS_T for the status.
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
msiDataObjUnlink( msParam_t *inpParam, msParam_t *outParam,
                  ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    dataObjInp_t dataObjInp, *myDataObjInp;
    char *outBadKeyWd = NULL;
    int validKwFlags;

    RE_TEST_MACRO( "    Calling msiDataObjUnlink" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjUnlink: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* parse inpParam */
    if ( strcmp( inpParam->type, STR_MS_T ) == 0 ) {
        bzero( &dataObjInp, sizeof( dataObjInp ) );
        myDataObjInp = &dataObjInp;
        validKwFlags = OBJ_PATH_FLAG | FORCE_FLAG_FLAG | REPL_NUM_FLAG |
                       RMTRASH_FLAG | ADMIN_RMTRASH_FLAG;
        rei->status = parseMsKeyValStrForDataObjInp( inpParam, myDataObjInp,
                      OBJ_PATH_KW, validKwFlags, &outBadKeyWd );
    }
    else {
        rei->status = parseMspForDataObjInp( inpParam, &dataObjInp,
                                             &myDataObjInp, 0 );
    }

    if ( rei->status < 0 ) {
        if ( outBadKeyWd != NULL ) {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjUnlink: input keyWd - %s error. status = %d",
                                outBadKeyWd, rei->status );
            free( outBadKeyWd );
        }
        else {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjUnlink: input msKeyValStr error. status = %d",
                                rei->status );
        }
        return rei->status;
    }

    rei->status = rsDataObjUnlink( rsComm, myDataObjInp );
    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjUnlink: rsDataObjUnlink failed for %s, status = %d",
                            myDataObjInp->objPath,
                            rei->status );
    }

    return rei->status;
}

/**
 * \fn msiDataObjRepl (msParam_t *inpParam1, msParam_t *msKeyValStr, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief  This microservice replicates a file in a Collection (it assigns a different
 *    replica number to the new copy in the iCAT Metadata Catalog).
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Mike Wan
 * \date    2007
 *
 * \note  Can be called by client through irule
 *
 * \note The replica is physically stored in the ‘tgReplResc’ Resource. *Junk contains
 * the status of the operation. In the Rule, the resource is provided as part
 * of the call instead of as an input through a *parameter.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - a msParam of type DataObjInp_MS_T or STR_MS_T which would be the obj Path.
 * \param[in] msKeyValStr - Optional - a STR_MS_T. This is the special msKeyValStr
 *      format of keyWd1=value1++++keyWd2=value2++++keyWd3=value3...
 *      If the keyWd is not specified (without the '=' char), the value is
 *      assumed to be the target resource ("destRescName") for backward
 *      compatibility.
 *    Valid keyWds are:
 *          \li "destRescName" - the target resource to replicate to.
 *          \li "backupRescName" - the target resource to backup
 *                the data. If this keyWd is used, the backup mode
 *                will be switched on.
 *          \li "rescName" - the resource of the source copy.
 *          \li "updateRepl" - update other replicas with the
 *                latest copy. This keyWd has no value. But
 *                the '=' character is still needed.
 *          \li "replNum" - the replica number to use as source.
 *          \li "numThreads" - the number of threads to use.
 *          \li "all" - replicate to all resources in the resource
 *                group. This keyWd has no value.
 *          \li "irodsAdmin" - admin user replicate other users' files.
 *                This keyWd has no value.
 *          \li "verifyChksum" - verify the transfer using checksum.
 *                This keyWd has no value.
 *          \li "rbudpTransfer" - use RBUDP (datagram) protocol for the
 *                data transfer. This keyWd has no value.
 *          \li "rbudpSendRate" - Valid only if "rbudpTransfer" is on. This
 *                is the send rate in kbits/sec. The default is 600,000.
 *          \li "rbudpPackSize" - Valid only if "rbudpTransfer" is on. This
 *                is the packet size in bytes. The default is 8192.
 * \param[out] outParam - a msParam of type INT_MS_T which is a status of the operation.
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
msiDataObjRepl( msParam_t *inpParam1, msParam_t *msKeyValStr,
                msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    dataObjInp_t dataObjInp, *myDataObjInp;
    transferStat_t *transStat = NULL;
    char *outBadKeyWd;
    int validKwFlags;

    RE_TEST_MACRO( "    Calling msiDataObjRepl" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjRepl: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* parse inpParam1 */
    rei->status = parseMspForDataObjInp( inpParam1, &dataObjInp,
                                         &myDataObjInp, 0 );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjRepl: input inpParam1 error. status = %d", rei->status );
        return rei->status;
    }

    validKwFlags = OBJ_PATH_FLAG | DEST_RESC_NAME_FLAG | NUM_THREADS_FLAG |
                   BACKUP_RESC_NAME_FLAG | RESC_NAME_FLAG | UPDATE_REPL_FLAG |
                   REPL_NUM_FLAG | ALL_FLAG | ADMIN_FLAG | VERIFY_CHKSUM_FLAG |
                   RBUDP_TRANSFER_FLAG | RBUDP_SEND_RATE_FLAG | RBUDP_PACK_SIZE_FLAG;
    rei->status = parseMsKeyValStrForDataObjInp( msKeyValStr, myDataObjInp,
                  DEST_RESC_NAME_KW, validKwFlags, &outBadKeyWd );

    if ( rei->status < 0 ) {
        if ( outBadKeyWd != NULL ) {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjRepl: input keyWd - %s error. status = %d",
                                outBadKeyWd, rei->status );
            free( outBadKeyWd );
        }
        else {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjRepl: input msKeyValStr error. status = %d",
                                rei->status );
        }
        return rei->status;
    }

    rei->status = rsDataObjRepl( rsComm, myDataObjInp, &transStat );

    if ( myDataObjInp == &dataObjInp ) {
        clearKeyVal( &myDataObjInp->condInput );
    }

    if ( transStat != NULL ) {
        free( transStat );
    }

    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjRepl: rsDataObjRepl failed %s, status = %d",
                            myDataObjInp->objPath,
                            rei->status );
    }

    return rei->status;
}

/**
 * \fn msiDataObjCopy (msParam_t *inpParam1, msParam_t *inpParam2,
 *  msParam_t *msKeyValStr, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * This microservice copies a file from one logical (source) collection to another
 * logical (destination) collection that is physically located in the input resource
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Michael Wan
 * \date    2007-02-12
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - a DataObjCopyInp_MS_T or
 *    DataObjInp_MS_T which is the source DataObjInp or
 *    STR_MS_T which would be the source object path.
 * \param[in] inpParam2 - Optional - a DataObjInp_MS_T which is the destination
 *    DataObjInp or STR_MS_T which would be the destination object path.
 * \param[in] msKeyValStr - Optional - a STR_MS_T. This is the special msKeyValStr
 *      format of keyWd1=value1++++keyWd2=value2++++keyWd3=value3...
 *      If the keyWd is not specified (without the '=' char), the value is
 *      assumed to be the target resource ("destRescName") for backward
 *      compatibility.
 *    Valid keyWds are:
 *             \li "destRescName" - the resource to copy to.
 *             \li "forceFlag" - overwrite existing copy. This keyWd has
                      no value. But the '=' character is still needed
 *             \li "numThreads" - the number of threads to use.
 *             \li "filePath" - The physical file path of the uploaded
 *                    file on the server.
 *             \li "dataType" - the data type of the file.
 *             \li "verifyChksum" - verify the transfer using checksum.
 *                    this keyWd has no value. But the '=' character is
 *                    still needed.
 * \param[out] outParam - a INT_MS_T for the status.
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
msiDataObjCopy( msParam_t *inpParam1, msParam_t *inpParam2,
                msParam_t *msKeyValStr, msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    dataObjCopyInp_t dataObjCopyInp, *myDataObjCopyInp;
    dataObjInp_t *myDataObjInp;
    transferStat_t *transStat = NULL;
    char *outBadKeyWd;
    int validKwFlags;

    RE_TEST_MACRO( "    Calling msiDataObjCopy" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjCopy: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;
    /* parse inpParam1 */
    rei->status = parseMspForDataObjCopyInp( inpParam1, &dataObjCopyInp, &myDataObjCopyInp );


    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjCopy: input inpParam1 error. status = %d", rei->status );
        return rei->status;
    }

    /* parse inpParam2 */
    rei->status = parseMspForDataObjInp( inpParam2, &myDataObjCopyInp->destDataObjInp, &myDataObjInp, 1 );


    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjCopy: input inpParam2 error. status = %d", rei->status );
        return rei->status;
    }

    validKwFlags = OBJ_PATH_FLAG   | DEST_RESC_NAME_FLAG | FILE_PATH_FLAG |
                   DATA_TYPE_FLAG  | VERIFY_CHKSUM_FLAG  |
                   FORCE_FLAG_FLAG | NUM_THREADS_FLAG;
    rei->status = parseMsKeyValStrForDataObjInp( msKeyValStr, &myDataObjCopyInp->destDataObjInp,
                  DEST_RESC_NAME_KW, validKwFlags, &outBadKeyWd );

    if ( rei->status < 0 ) {
        if ( outBadKeyWd != NULL ) {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjCopy: input keyWd - %s error. status = %d",
                                outBadKeyWd, rei->status );
            free( outBadKeyWd );
        }
        else {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjCopy: input msKeyValStr error. status = %d",
                                rei->status );
        }
        return rei->status;
    }

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjCopy: input inpParam2 error. status = %d", rei->status );
        return rei->status;
    }

    rei->status = rsDataObjCopy( rsComm, myDataObjCopyInp, &transStat );
    if ( transStat != NULL ) {
        free( transStat );
    }

    if ( myDataObjCopyInp == &dataObjCopyInp ) {
        clearKeyVal( &myDataObjCopyInp->destDataObjInp.condInput );
    }

    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjCopy: rsDataObjCopy failed for %s, status = %d",
                            myDataObjCopyInp->srcDataObjInp.objPath,
                            rei->status );
    }

    return rei->status;
}

/**
 * \fn msiDataObjPut (msParam_t *inpParam1, msParam_t *inpParam2,
 * msParam_t *msKeyValStr, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice requests the client to call a rcDataObjPut API
 *   as part of a workflow execution.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Michael Wan
 * \date    2007-02-12
 *
 * \note This call should only be used through the rcExecMyRule (irule) call
 *  i.e., rule execution initiated by clients and should not be called
 *  internally by the server since it interacts with the client through
 *  the normal client/server socket connection. Also, it should never
 *  be called through delayExec since it requires client interaction.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - A DataObjInp_MS_T or STR_MS_T which would be taken as dataObj path.
 * \param[in] inpParam2 - Optional - a STR_MS_T which specifies the resource.
 * \param[in] msKeyValStr - Optional - a STR_MS_T. This is the special msKeyValStr
 *      format of keyWd1=value1++++keyWd2=value2++++keyWd3=value3...
 *      If the keyWd is not specified (without the '=' char), the value is
 *      assumed to be the client's local file path ("localPath") for backward
 *      compatibility.
 *      Valid keyWds are:
 *          \li "localPath" - the client's local file path.
 *          \li "destRescName" - the target resource - where the object should go.
 *          \li "all" - upload to all resources
 *          \li "forceFlag" - overwrite existing copy. This keyWd has
 *                no value. But the '=' character is still needed
 *          \li "replNum" - the replica number to overwrite.
 *          \li "numThreads" - the number of threads to use.
 *          \li "filePath" - The physical file path of the uploaded
 *                file on the server.
 *          \li "dataType" - the data type of the file.
 *          \li "verifyChksum" - verify the transfer using checksum.
 *                this keyWd has no value. But the '=' character is
 *                still needed.
 * \param[out] outParam - a INT_MS_T containing the status.
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
msiDataObjPut( msParam_t *inpParam1, msParam_t *inpParam2,
               msParam_t *msKeyValStr, msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    dataObjInp_t *myDataObjInp = NULL;
    msParamArray_t *myMsParamArray;
    char *outBadKeyWd;
    int validKwFlags;

    RE_TEST_MACRO( "    Calling msiDataObjPut" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjPut: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* parse inpParam1 */
    dataObjInp_t *dataObjInp = ( dataObjInp_t* )malloc( sizeof( *dataObjInp ) );
    rei->status = parseMspForDataObjInp( inpParam1, dataObjInp,
                                         &myDataObjInp, 1 );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjPut: input inpParam1 error. status = %d", rei->status );
        clearDataObjInp( dataObjInp );
        free( dataObjInp );
        return rei->status;
    }

    rei->status = parseMspForCondInp( inpParam2, &dataObjInp->condInput,
                                      DEST_RESC_NAME_KW );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjPut: input inpParam2 error. status = %d", rei->status );
        clearDataObjInp( dataObjInp );
        free( dataObjInp );
        return rei->status;
    }

    validKwFlags = LOCAL_PATH_FLAG | DEST_RESC_NAME_FLAG | FILE_PATH_FLAG |
                   REPL_NUM_FLAG | DATA_TYPE_FLAG | VERIFY_CHKSUM_FLAG |
                   ALL_FLAG | FORCE_FLAG_FLAG | NUM_THREADS_FLAG | OBJ_PATH_FLAG;
    rei->status = parseMsKeyValStrForDataObjInp( msKeyValStr, dataObjInp,
                  LOCAL_PATH_KW, validKwFlags, &outBadKeyWd );

    if ( rei->status < 0 ) {
        if ( outBadKeyWd != NULL ) {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjPut: input keyWd - %s error. status = %d",
                                outBadKeyWd, rei->status );
            free( outBadKeyWd );
        }
        else {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjPut: input msKeyValStr error. status = %d",
                                rei->status );
        }
        clearDataObjInp( dataObjInp );
        free( dataObjInp );
        return rei->status;
    }

    myMsParamArray = ( msParamArray_t* )malloc( sizeof( msParamArray_t ) );
    memset( myMsParamArray, 0, sizeof( msParamArray_t ) );

    rei->status = addMsParam( myMsParamArray, CL_PUT_ACTION, DataObjInp_MS_T,
                              ( void * ) dataObjInp, NULL );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjPut: addMsParam error. status = %d", rei->status );
        clearMsParamArray( myMsParamArray, 0 );
        free( myMsParamArray );
        clearDataObjInp( dataObjInp );
        free( dataObjInp );
        return rei->status;
    }

    /* tell the client to do the put */
    rei->status = sendAndRecvBranchMsg( rsComm, rsComm->apiInx,
                                        SYS_SVR_TO_CLI_MSI_REQUEST, ( void * ) myMsParamArray, NULL );

    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjPut: rsDataObjPut failed for %s, status = %d",
                            dataObjInp->objPath,
                            rei->status );
    }

    return rei->status;
}

/**
 * \fn msiDataObjGet (msParam_t *inpParam1, msParam_t *msKeyValStr,
 * msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice requests the client to call a rcDataObjGet API
 *   as part of a workflow execution.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Michael Wan
 * \date    2007-02-12
 *
 * \note This call should only be used through the rcExecMyRule (irule) call
 *  i.e., rule execution initiated by clients and should not be called
 *  internally by the server since it interacts with the client through
 *  the normal client/server socket connection. Also, it should never
 *  be called through delayExec since it requires client interaction.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - A DataObjInp_MS_T or STR_MS_T which would be taken as dataObj path.
 * \param[in] msKeyValStr - Optional - a STR_MS_T. This is the special msKeyValStr
 *      format of keyWd1=value1++++keyWd2=value2++++keyWd3=value3...
 *      If the keyWd is not specified (without the '=' char), the value is
 *      assumed to be the client's local file path ("localPath") for backward
 *      compatibility..
 *      Valid keyWds are:
 *          \li "localPath" - the client's local file path.
 *          \li "rescName" - the resource of the copy to get.
 *          \li "replNum" - the replica number of the copy to get.
 *          \li "numThreads" - the number of threads to use.
 *          \li "forceFlag" - overwrite local copy. This keyWd has
 *                no value. But the '=' character is still needed
 *          \li "verifyChksum" - verify the transfer using checksum.
 *                this keyWd has no value. But the '=' character is
 *                still needed.
 * \param[out] outParam - a INT_MS_T containing the status.
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
msiDataObjGet( msParam_t *inpParam1, msParam_t *msKeyValStr,
               msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    dataObjInp_t *myDataObjInp = NULL;
    msParamArray_t *myMsParamArray;
    char *outBadKeyWd = NULL;
    int validKwFlags;

    RE_TEST_MACRO( "    Calling msiDataObjGet" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjGet: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* parse inpParam1 */
    dataObjInp_t *dataObjInp = ( dataObjInp_t* )malloc( sizeof( *dataObjInp ) );
    rei->status = parseMspForDataObjInp( inpParam1, dataObjInp,
                                         &myDataObjInp, 1 );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjGet: input inpParam1 error. status = %d",
                            rei->status );
        clearDataObjInp( dataObjInp );
        free( dataObjInp );
        return rei->status;
    }

    validKwFlags = LOCAL_PATH_FLAG | FORCE_FLAG_FLAG | NUM_THREADS_FLAG |
                   RESC_NAME_FLAG | REPL_NUM_FLAG | VERIFY_CHKSUM_FLAG | OBJ_PATH_FLAG;
    rei->status = parseMsKeyValStrForDataObjInp( msKeyValStr, dataObjInp,
                  LOCAL_PATH_KW, validKwFlags, &outBadKeyWd );

    if ( rei->status < 0 ) {
        if ( outBadKeyWd != NULL ) {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjGet: input keyWd - %s error. status = %d",
                                outBadKeyWd, rei->status );
            free( outBadKeyWd );
        }
        else {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjGet: input msKeyValStr error. status = %d",
                                rei->status );
        }
        clearDataObjInp( dataObjInp );
        free( dataObjInp );
        return rei->status;
    }

    myMsParamArray = ( msParamArray_t* )malloc( sizeof( msParamArray_t ) );
    memset( myMsParamArray, 0, sizeof( msParamArray_t ) );

    rei->status = addMsParam( myMsParamArray, CL_GET_ACTION, DataObjInp_MS_T,
                              ( void * ) dataObjInp, NULL );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjGet: addMsParam error. status = %d",
                            rei->status );
        clearMsParamArray( myMsParamArray, 0 );
        free( myMsParamArray );
        clearDataObjInp( dataObjInp );
        free( dataObjInp );
        return rei->status;
    }

    /* tell the client to do the get */
    rei->status = sendAndRecvBranchMsg( rsComm, rsComm->apiInx,
                                        SYS_SVR_TO_CLI_MSI_REQUEST, ( void * ) myMsParamArray, NULL );

    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjGet: rsDataObjGet failed for %s, status = %d",
                            dataObjInp->objPath, rei->status );
    }

    return rei->status;
}

/**
 * \fn msiDataObjGetWithOptions (msParam_t *inpParam1, msParam_t *inpParam2,
 * msParam_t *srcrescParam, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice requests the client to call a rcDataObjGet API
 *   as part of a workflow execution, with options.
 *
 * \deprecated Since 2.2, #msiDataObjGet can take a parameter with key-values format which make this microservice obsolete.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Michael Wan, modified by Romain GUINOT
 * \date    2007-02-12
 *
 * \note This call should only be used through the rcExecMyRule (irule) call
 *  i.e., rule execution initiated by clients and should not be called
 *  internally by the server since it interacts with the client through
 *  the normal client/server socket connection. Also, it should never
 *  be called through delayExec since it requires client interaction.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - A DataObjInp_MS_T or STR_MS_T which would be taken as dataObj path.
 * \param[in] inpParam2 - Optional - a STR_MS_T which specifies the client's local file path.
 * \param[in] srcrescParam - Optional - a STR_MS_T which specifies the source resource.
 * \param[out] outParam - a INT_MS_T containing the status.
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
 * \pre none
 * \post none
 * \sa none
**/
int
msiDataObjGetWithOptions( msParam_t *inpParam1, msParam_t *inpParam2,
                          msParam_t *srcrescParam, msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    dataObjInp_t *myDataObjInp = NULL;
    msParamArray_t *myMsParamArray;

    RE_TEST_MACRO( "    Calling msiDataObjGetWithOptions" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjGetWithOptions: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* parse inpParam1 */
    dataObjInp_t *dataObjInp = ( dataObjInp_t* )malloc( sizeof( *dataObjInp ) );
    rei->status = parseMspForDataObjInp( inpParam1, dataObjInp,
                                         &myDataObjInp, 1 );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjGetWithOptions: input inpParam1 error. status = %d",
                            rei->status );
        return rei->status;
    }

    rei->status = parseMspForCondInp( inpParam2, &dataObjInp->condInput,
                                      LOCAL_PATH_KW );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjGetWithOptions: input inpParam2 error. status = %d",
                            rei->status );
        clearDataObjInp( dataObjInp );
        free( dataObjInp );
        return rei->status;
    }

    rei->status = parseMspForCondInp( srcrescParam, &dataObjInp->condInput,
                                      RESC_NAME_KW );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjGetWithOptions: input srcrescParam error. status = %d",
                            rei->status );
        clearDataObjInp( dataObjInp );
        free( dataObjInp );
        return rei->status;
    }

    myMsParamArray = ( msParamArray_t* )malloc( sizeof( msParamArray_t ) );
    memset( myMsParamArray, 0, sizeof( msParamArray_t ) );

    rei->status = addMsParam( myMsParamArray, CL_GET_ACTION, DataObjInp_MS_T,
                              ( void * ) dataObjInp, NULL );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjGetWithOptions: addMsParam error. status = %d",
                            rei->status );
        clearMsParamArray( myMsParamArray, 0 );
        free( myMsParamArray );
        clearDataObjInp( dataObjInp );
        free( dataObjInp );
        return rei->status;
    }

    /* tell the client to do the get */
    rei->status = sendAndRecvBranchMsg( rsComm, rsComm->apiInx,
                                        SYS_SVR_TO_CLI_MSI_REQUEST, ( void * ) myMsParamArray, NULL );

    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjGetWithOptions: rsDataObjGet failed, status = %d",
                            rei->status );
    }

    return rei->status;
}

/**
* \fn msiDataObjChksum (msParam_t *inpParam1, msParam_t *msKeyValStr, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice calls rsDataObjChksum to chksum the iput data object as part of a workflow execution.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Michael Wan
 * \date    2007-04-02
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - A DataObjInp_MS_T or a STR_MS_T which would be taken as dataObj path.
 * \param[in] msKeyValStr - Optional - a STR_MS_T. This is the special msKeyValStr
 *      format of keyWd1=value1++++keyWd2=value2++++keyWd3=value3...
 *      If the keyWd is not specified (without the '=' char), the value is
 *      assumed to be the target resource ("destRescName") for backward
 *      compatibility.
 *      Valid keyWds are:
 *        \li "ChksumAll" - checksum all replicas. This keyWd has no value.
 *            But the '=' character is still needed.
 *        \li "verifyChksum" - verify the chksum value.
 *        \li "forceChksum" - checksum data-objects even if a
 *            checksum already exists in iCAT. This keyWd has no value.
 *        \li "replNum" - the replica number to checksum. This keyWd has no value.
 * \param[out] outParam - a STR_MS_T containing the chksum value.
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
 * \pre none
 * \post none
 * \sa none
**/
int
msiDataObjChksum( msParam_t *inpParam1, msParam_t *msKeyValStr,
                  msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    dataObjInp_t dataObjInp, *myDataObjInp = NULL;
    char *chksum = NULL;
    char *outBadKeyWd;
    int validKwFlags;

    RE_TEST_MACRO( "    Calling msiDataObjChksum" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjChksum: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* parse inpParam1 */
    rei->status = parseMspForDataObjInp( inpParam1, &dataObjInp,
                                         &myDataObjInp, 1 );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjChksum: input inpParam1 error. status = %d", rei->status );
        return rei->status;
    }

    validKwFlags = CHKSUM_ALL_FLAG | FORCE_CHKSUM_FLAG | REPL_NUM_FLAG |
                   OBJ_PATH_FLAG | VERIFY_CHKSUM_FLAG;
    if ( ( rei->status = parseMsKeyValStrForDataObjInp( msKeyValStr,
                         myDataObjInp, KEY_WORD_KW, validKwFlags, &outBadKeyWd ) ) < 0 ) {
        if ( outBadKeyWd != NULL ) {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjChksum: input keyWd - %s error. status = %d",
                                outBadKeyWd, rei->status );
            free( outBadKeyWd );
        }
        else {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiDataObjChksum: input msKeyValStr error. status = %d",
                                rei->status );
        }
        return rei->status;
    }

    rei->status = rsDataObjChksum( rsComm, myDataObjInp, &chksum );

    if ( myDataObjInp == &dataObjInp ) {
        clearKeyVal( &myDataObjInp->condInput );
    }

    if ( rei->status >= 0 ) {
        fillStrInMsParam( outParam, chksum );
        free( chksum );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjChksum: rsDataObjChksum failed for %s, status = %d",
                            myDataObjInp->objPath ,
                            rei->status );
    }

    return rei->status;
}

/**
 * \fn msiDataObjPhymv (msParam_t *inpParam1, msParam_t *inpParam2,
 * msParam_t *inpParam3, msParam_t *inpParam4, msParam_t *inpParam5,
 * msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice calls rsDataObjPhymv to physically move the iput
 *    data object to another resource.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Michael Wan
 * \date    2007-04-02
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - A DataObjInp_MS_T or STR_MS_T which would be taken as dataObj path.
 * \param[in] inpParam2 - Optional - a STR_MS_T which specifies the dest resourceName.
 * \param[in] inpParam3 - Optional - a STR_MS_T which specifies the src resourceName.
 * \param[in] inpParam4 - Optional - a STR_MS_T which specifies the replNum.
 * \param[in] inpParam5 - Optional - a STR_MS_T which specifies the ADMIN_KW.
 * \param[out] outParam - a INT_MS_T containing the status.
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
 * \pre none
 * \post none
 * \sa none
**/
int
msiDataObjPhymv( msParam_t *inpParam1, msParam_t *inpParam2,
                 msParam_t *inpParam3, msParam_t *inpParam4, msParam_t *inpParam5,
                 msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    dataObjInp_t dataObjInp, *myDataObjInp = NULL;
    transferStat_t *transStat = NULL;

    RE_TEST_MACRO( "    Calling msiDataObjPhymv" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjPhymv: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* parse inpParam1 */

    if ( ( rei->status = parseMspForDataObjInp( inpParam1, &dataObjInp,
                         &myDataObjInp, 0 ) ) < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjPhymv: input inpParam1 error. status = %d", rei->status );
        return rei->status;
    }

    if ( ( rei->status = parseMspForCondInp( inpParam2, &myDataObjInp->condInput,
                         DEST_RESC_NAME_KW ) ) < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjPhymv: input inpParam2 error. status = %d", rei->status );
        return rei->status;
    }

    if ( ( rei->status = parseMspForCondInp( inpParam3, &myDataObjInp->condInput,
                         RESC_NAME_KW ) ) < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjPhymv: input inpParam3 error. status = %d", rei->status );
        return rei->status;
    }

    if ( ( rei->status = parseMspForCondInp( inpParam4, &myDataObjInp->condInput,
                         REPL_NUM_KW ) ) < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjPhymv: input inpParam4 error. status = %d", rei->status );
        return rei->status;
    }

    if ( ( rei->status = parseMspForCondInp( inpParam5, &myDataObjInp->condInput,
                         ADMIN_KW ) ) < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjPhymv: input inpParam5 error. status = %d", rei->status );
        return rei->status;
    }

    rei->status = rsDataObjPhymv( rsComm, myDataObjInp, &transStat );

    if ( transStat != NULL ) {
        free( transStat );
    }

    if ( myDataObjInp == &dataObjInp ) {
        clearKeyVal( &myDataObjInp->condInput );
    }

    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjPhymv: rsDataObjPhymv failed for %s, status = %d",
                            myDataObjInp->objPath,
                            rei->status );
    }

    return rei->status;
}

/**
 * \fn msiDataObjRename (msParam_t *inpParam1, msParam_t *inpParam2,
 * msParam_t *inpParam3, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice calls rsDataObjRename to rename the iput
 *     data object or collection to another path.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Michael Wan
 * \date    2007-04-02
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - A taObjCopyInp_MS_T or STR_MS_T which would be taken as the src dataObj path.
 * \param[in] inpParam2 - Optional - A DataObjInp_MS_T which is the destination
 *              DataObjInp or STR_MS_T which would be the destination object Path.
 * \param[in] inpParam3 - Optional - a INT_MS_T or STR_MS_T which specifies the
 *              object type. A 0 means data obj and > 0 mean collection.
 * \param[out] outParam - a INT_MS_T containing the status.
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
 * \pre none
 * \post none
 * \sa none
**/
int
msiDataObjRename( msParam_t *inpParam1, msParam_t *inpParam2,
                  msParam_t *inpParam3, msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    dataObjCopyInp_t dataObjRenameInp, *myDataObjRenameInp;
    dataObjInp_t *myDataObjInp = NULL;

    RE_TEST_MACRO( "    Calling msiDataObjRename" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjRename: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* parse inpParam1 */
    rei->status = parseMspForDataObjCopyInp( inpParam1, &dataObjRenameInp,
                  &myDataObjRenameInp );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjRename: input inpParam1 error. status = %d", rei->status );
        return rei->status;
    }

    /* parse inpParam2 */
    rei->status = parseMspForDataObjInp( inpParam2,
                                         &myDataObjRenameInp->destDataObjInp, &myDataObjInp, 1 );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjRename: input inpParam2 error. status = %d", rei->status );
        return rei->status;
    }

    if ( inpParam3 != NULL ) {
        int myInt;
        myInt = parseMspForPosInt( inpParam3 );
        if ( myInt > 0 ) {
            myDataObjRenameInp->srcDataObjInp.oprType = RENAME_COLL;
        }
        else {
            myDataObjRenameInp->srcDataObjInp.oprType = RENAME_DATA_OBJ;
        }
    }

    rei->status = rsDataObjRename( rsComm, myDataObjRenameInp );

    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjRename: rsDataObjRename failed for %s, status = %d",
                            myDataObjRenameInp->srcDataObjInp.objPath,
                            rei->status );
    }

    return rei->status;
}

/**
 * \fn msiDataObjTrim (msParam_t *inpParam1, msParam_t *inpParam2,
 *      msParam_t *inpParam3, msParam_t *inpParam4, msParam_t *inpParam5,
 *      msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice calls rsDataObjTrim to trim down the number
 *          of replicas of a data object.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Michael Wan
 * \date    2007-04-02
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - A DataObjInp_MS_T or STR_MS_T which would be taken as dataObj path.
 * \param[in] inpParam2 - Optional - a STR_MS_T which specifies the resourceName.
 * \param[in] inpParam3 - Optional - a STR_MS_T which specifies the replNum.
 * \param[in] inpParam4 - Optional - a STR_MS_T which specifies the minimum number of copies to keep.
 * \param[in] inpParam5 - Optional - a STR_MS_T which specifies the ADMIN_KW.
 * \param[out] outParam - a INT_MS_T containing the status.
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
 * \pre none
 * \post none
 * \sa none
**/
int
msiDataObjTrim( msParam_t *inpParam1, msParam_t *inpParam2,
                msParam_t *inpParam3, msParam_t *inpParam4, msParam_t *inpParam5,
                msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    dataObjInp_t dataObjInp, *myDataObjInp = NULL;

    RE_TEST_MACRO( "    Calling msiDataObjTrim" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjTrim: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* parse inpParam1 */

    if ( ( rei->status = parseMspForDataObjInp( inpParam1, &dataObjInp,
                         &myDataObjInp, 0 ) ) < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjTrim: input inpParam1 error. status = %d", rei->status );
        return rei->status;
    }

    if ( ( rei->status = parseMspForCondInp( inpParam2, &myDataObjInp->condInput,      RESC_NAME_KW ) ) < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjTrim: input inpParam2 error. status = %d", rei->status );
        return rei->status;
    }

    if ( ( rei->status = parseMspForCondInp( inpParam3, &myDataObjInp->condInput,
                         REPL_NUM_KW ) ) < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjTrim: input inpParam3 error. status = %d", rei->status );
        return rei->status;
    }

    if ( ( rei->status = parseMspForCondInp( inpParam4, &myDataObjInp->condInput,
                         COPIES_KW ) ) < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjTrim: input inpParam4 error. status = %d", rei->status );
        return rei->status;
    }

    if ( ( rei->status = parseMspForCondInp( inpParam5, &myDataObjInp->condInput,
                         ADMIN_KW ) ) < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjTrim: input inpParam5 error. status = %d", rei->status );
        return rei->status;
    }

    rei->status = rsDataObjTrim( rsComm, myDataObjInp );

    if ( myDataObjInp == &dataObjInp ) {
        clearKeyVal( &myDataObjInp->condInput );
    }

    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjTrim: rsDataObjTrim failed for %s, status = %d",
                            myDataObjInp->objPath,
                            rei->status );
    }

    return rei->status;
}

/**
 * \fn msiCollCreate (msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice calls rsCollCreate to create a collection
 *    as part of a workflow execution.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Michael Wan
 * \date    2007-04-02
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - a CollInp_MS_T or a STR_MS_T which
 *    would be taken as dataObj path.
 * \param[in] inpParam2 - a STR_MS_T which specifies the flags integer. A
 *    flags value of 1 means the parent collections will be created too.
 * \param[out] outParam - a INT_MS_T containing the status.
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
 * \pre none
 * \post none
 * \sa none
**/
int
msiCollCreate( msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    collInp_t collCreateInp, *myCollCreateInp;
    int flags;

    RE_TEST_MACRO( "    Calling msiCollCreate" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiCollCreate: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* parse inpParam1 */
    rei->status = parseMspForCollInp( inpParam1, &collCreateInp,
                                      &myCollCreateInp, 1 );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiCollCreate: input inpParam1 error. status = %d", rei->status );
        return rei->status;
    }

    flags = parseMspForPosInt( inpParam2 );

    if ( flags > 0 ) {
        addKeyVal( &collCreateInp.condInput, RECURSIVE_OPR__KW, "" );
    }
    rei->status = rsCollCreate( rsComm, myCollCreateInp );

    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiCollCreate: rsCollCreate failed %s, status = %d",
                            collCreateInp.collName,
                            rei->status );
    }

    if ( myCollCreateInp == &collCreateInp ) {
        clearKeyVal( &myCollCreateInp->condInput );
    }

    return rei->status;
}


/**
 * \fn msiRmColl (msParam_t *inpParam1, msParam_t *msKeyValStr, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice calls rsRmColl to recursively remove a collection
 *    as part of a workflow execution.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Michael Wan
 * \date    2007-04-02
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - a CollInp_MS_T or a STR_MS_T which would be taken as dataObj path.
 * \param[in] msKeyValStr - This is the special msKeyValStr
 *   format of keyWd1=value1++++keyWd2=value2++++keyWd3=value3...
 *   If the keyWd is not specified (without the '=' char), the value is
 *   assumed to be one of the keywords listed below for backwards
 *   compatibility.
 *   Valid keyWds are :
 *        \li "forceFlag" - Remove the data object instead of putting
 *                      it in the trash. This keyWd has no value. But the
 *                      '=' character is still needed.
 *        \li "irodsAdminRmTrash" - Admin remove trash. This keyWd has no value.
 *        \li "irodsRmTrash" - Remove trash. This keyWd has no value.
 * \param[out] outParam - an INT_MS_T containing the status.
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
 * \pre none
 * \post none
 * \sa none
**/
int
msiRmColl( msParam_t *inpParam1, msParam_t *msKeyValStr, msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    collInp_t rmCollInp, *myRmCollInp;
    char *outBadKeyWd;
    int validKwFlags;

    RE_TEST_MACRO( "    Calling msiRmColl" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiRmColl: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* parse inpParam1 */
    rei->status = parseMspForCollInp( inpParam1, &rmCollInp,
                                      &myRmCollInp, 1 );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiRmColl: input inpParam1 error. status = %d", rei->status );
        return rei->status;
    }

    validKwFlags = RMTRASH_FLAG | ADMIN_RMTRASH_FLAG |
                   FORCE_FLAG_FLAG | COLL_NAME_FLAG;
    if ( ( rei->status = parseMsKeyValStrForCollInp( msKeyValStr,
                         myRmCollInp, KEY_WORD_KW, validKwFlags, &outBadKeyWd ) ) < 0 ) {
        if ( outBadKeyWd != NULL ) {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiRmColl: input keyWd - %s error. status = %d",
                                outBadKeyWd, rei->status );
            free( outBadKeyWd );
        }
        else {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiRmColl: input msKeyValStr error. status = %d",
                                rei->status );
        }
        return rei->status;
    }

    addKeyVal( &myRmCollInp->condInput, RECURSIVE_OPR__KW, "" );

    rei->status = rsRmColl( rsComm, myRmCollInp, NULL );

    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiRmColl: rsRmColl failed for %s, status = %d",
                            myRmCollInp->collName,
                            rei->status );
    }

    if ( myRmCollInp == &rmCollInp ) {
        clearKeyVal( &myRmCollInp->condInput );
    }

    return rei->status;
}


/**
 * \fn msiReplColl (msParam_t *coll, msParam_t *destRescName, msParam_t *options,
 *  msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice iterates through a collection and calls
 *  rsDataObjRepl to recursively replicate the collection
 *  as part of a workflow execution.
 *
 * \deprecated The new microservice for replicating a collection is #msiCollRepl.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Sifang Lu
 * \date    2007-10-01
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] coll - Required - A CollInp_MS_T or a STR_MS_T which would be taken
 *               as destination collection path.
 * \param[in] destRescName - A STR_MS_T destination resource name.
 * \param[in] options - A STR_MS_T - a group of options in a string delimited by '%%'.
 *               If the string is empty ("\0") or null ("NULL") it will not
 *               be used.
 *               The options can be the following:
 *              \li "all"(ALL_KW)
 *              \li "irodsAdmin" (ADMIN_KW).
 *              \li "backupMode" if specified, it will try to use 'backup mode'
 *                to the destination resource. Means if a good copy already
 *                exists in destination resource, it will not throw an error
 * \param[out] outParam - an INT_MS_T containing the status.
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
 * \pre
 * \post
 * \sa
**/
int
msiReplColl( msParam_t *coll, msParam_t *destRescName, msParam_t *options,
             msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    collInp_t collInp, *myCollInp;
    int i, continueInx, status;
    transferStat_t *transStat = NULL;
    strArray_t optArray;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    dataObjInp_t dataObjInp;

    RE_TEST_MACRO( "    Calling msiReplColl" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiReplColl: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* parse inpParam1: coll */
    rei->status = parseMspForCollInp( coll, &collInp,
                                      &myCollInp, 0 );
    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiReplColl: input inpParam1 error. status = %d", rei->status );
        return rei->status;
    }

    /* parse inpParam2: destRescName, and assign the destination
       resource to dataobjinp */
    memset( &dataObjInp, 0, sizeof( dataObjInp_t ) );
    rei->status = parseMspForCondInp( destRescName,
                                      &( &dataObjInp )->condInput, DEST_RESC_NAME_KW );
    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiReplColl: input inpParam2 error. status = %d", rei->status );
        return rei->status;
    }

    /* parse inpParam3: options, and assign the then to conditional
       keywords */
    if ( ( strlen( ( char* )options->inOutStruct ) > 0 ) &&
            ( 0 != strcmp( ( char* )options->inOutStruct, "null" ) ) ) {
        memset( &optArray, 0, sizeof( optArray ) );
        status = parseMultiStr( ( char * )options->inOutStruct, &optArray );
        if ( status <= 0 ) {
            rodsLog( LOG_ERROR,
                     "msiReplColl: Could not parse options string '%s'",
                     options->inOutStruct );
        }
        for ( i = 0; i < optArray.len; i++ ) {
            char *option;
            option = &optArray.value[i * optArray.size];
            if ( strcmp( option, ALL_KW ) &&
                    strcmp( option, ADMIN_KW ) &&
                    strcmp( option, "backupMode" )
               ) {
                rodsLog( LOG_ERROR, "msiReplColl: invalid option: '%s'", option );
                continue;
            }
            if ( strcmp( option, "backupMode" ) == 0 )
                addKeyVal( &( dataObjInp.condInput ), BACKUP_RESC_NAME_KW,
                           ( char * )destRescName->inOutStruct );
            else {
                addKeyVal( &( dataObjInp.condInput ), option, "" );
            }
        }
    }

    /* iterate through all files */
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    status = rsQueryDataObjInCollReCur( rsComm, myCollInp->collName,
                                        &genQueryInp, &genQueryOut, NULL, 1 );
    if ( status < 0 && status != CAT_NO_ROWS_FOUND ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiReplColl: msiReplColl error for %s, stat=%d",
                            myCollInp->collName, status );
        rei->status = status;
        return rei->status;
    }
    while ( rei->status >= 0 ) {
        sqlResult_t *subColl, *dataObj;
        /* get sub coll paths in the batch */
        if ( ( subColl = getSqlResultByInx( genQueryOut, COL_COLL_NAME ) )
                == NULL ) {
            rodsLog( LOG_ERROR,
                     "msiReplColl: msiReplColl for COL_COLL_NAME failed" );
            rei->status = UNMATCHED_KEY_OR_INDEX;
            return rei->status;
        }
        /* get data names in the batch */
        if ( ( dataObj = getSqlResultByInx( genQueryOut, COL_DATA_NAME ) )
                == NULL ) {
            rodsLog( LOG_ERROR,
                     "msiReplColl: msiReplColl for COL_DATA_NAME failed" );
            rei->status = UNMATCHED_KEY_OR_INDEX;
            return rei->status;
        }

        for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
            char *tmpSubColl, *tmpDataName;

            tmpSubColl = &subColl->value[subColl->len * i];
            tmpDataName = &dataObj->value[dataObj->len * i];
            snprintf( dataObjInp.objPath, MAX_NAME_LEN, "%s/%s", tmpSubColl, tmpDataName );

            // =-=-=-=-=-=-=-
            // necessary to clear these as an incoming resc hier will be used
            // rather than querying for a new one during the repl
            rmKeyVal( &dataObjInp.condInput, RESC_HIER_STR_KW );
            rmKeyVal( &dataObjInp.condInput, DEST_RESC_HIER_STR_KW );

            rei->status = rsDataObjRepl( rsComm, &dataObjInp, &transStat );
            if ( rei->status < 0 ) {
                rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                    "msiReplColl: rsDataObjRepl failed %s, status = %d",
                                    ( &dataObjInp )->objPath,
                                    rei->status );
            }
            if ( transStat != NULL ) {
                free( transStat );
            }
        }

        continueInx = genQueryOut->continueInx;
        freeGenQueryOut( &genQueryOut );
        if ( continueInx > 0 ) {
            /* More to come */
            genQueryInp.continueInx = continueInx;
            rei->status =  rsGenQuery( rsComm, &genQueryInp, &genQueryOut );
        }
        else {
            break;
        }
    }

    clearKeyVal( &dataObjInp.condInput );

    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "dataObjInp: msiReplColl failed (should have catched earlier) %s, status = %d",
                            ( &dataObjInp )->objPath,
                            rei->status );
    }
    return rei->status;
}

/**
 * \fn msiPhyPathReg (msParam_t *inpParam1, msParam_t *inpParam2,
 * msParam_t *inpParam3, msParam_t *inpParam4, msParam_t *outParam,
 * ruleExecInfo_t *rei)
 *
 * \brief This microservice calls rsPhyPathReg to register a physical path
 * with the iCAT.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Michael Wan
 * \date    2007-04-02
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - A DataObjInp_MS_T or STR_MS_T which would be taken
 *    as object path. The path can be a data object or a collection path.
 * \param[in] inpParam2 - Optional - a STR_MS_T which specifies the dest resourceName.
 * \param[in] inpParam3 - Optional - a STR_MS_T which specifies the physical path to be registered.
 * \param[in] inpParam4 - Optional - a STR_MS_T which specifies whether the
 *    path to be registered is a directory. A keyword string "collection"
 *    indicates the path is a directory. A "null" string indicates the path
 *    is a file.  A "mountPoint" (MOUNT_POINT_STR) means mounting the file
 *      directory given in inpParam3. A "linkPoint" (LINK_POINT_STR)
 *      means soft link the collection given in inpParam3.
 * \param[out] outParam - a INT_MS_T containing the status.
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
 * \pre none
 * \post none
 * \sa none
**/
int
msiPhyPathReg( msParam_t *inpParam1, msParam_t *inpParam2,
               msParam_t *inpParam3, msParam_t *inpParam4, msParam_t *outParam,
               ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    dataObjInp_t dataObjInp, *myDataObjInp = NULL;

    RE_TEST_MACRO( "    Calling msiPhyPathReg" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiPhyPathReg: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* parse inpParam1 */
    rei->status = parseMspForDataObjInp( inpParam1, &dataObjInp,
                                         &myDataObjInp, 1 );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiPhyPathReg: input inpParam1 error. status = %d", rei->status );
        return rei->status;
    }

    if ( ( rei->status = parseMspForCondInp( inpParam2, &myDataObjInp->condInput,
                         DEST_RESC_NAME_KW ) ) < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjPhymv: input inpParam2 error. status = %d", rei->status );
        return rei->status;
    }

    if ( ( rei->status = parseMspForCondInp( inpParam3, &myDataObjInp->condInput,
                         FILE_PATH_KW ) ) < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjPhymv: input inpParam3 error. status = %d", rei->status );
        return rei->status;
    }

    if ( ( rei->status = parseMspForPhyPathReg( inpParam4,
                         &myDataObjInp->condInput ) ) < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiPhyPathReg: input inpParam4 error. status = %d", rei->status );
        return rei->status;
    }

    rei->status = rsPhyPathReg( rsComm, myDataObjInp );

    if ( myDataObjInp == &dataObjInp ) {
        clearKeyVal( &myDataObjInp->condInput );
    }

    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiPhyPathReg: rsPhyPathReg failed for %s, status = %d",
                            myDataObjInp->objPath,
                            rei->status );
    }

    return rei->status;
}

/**
 * \fn msiObjStat (msParam_t *inpParam1, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice calls rsObjStat to get the stat of an iRODS path
 *  as part of a workflow execution.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Michael Wan
 * \date    2007-04-02
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - A DataObjInp_MS_T or STR_MS_T which would be taken as dataObj path.
 * \param[out] outParam - a RodsObjStat_MS_T containing the rodsObjStat_t struct of the object.
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
msiObjStat( msParam_t *inpParam1, msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    dataObjInp_t dataObjInp, *myDataObjInp = NULL;

    RE_TEST_MACRO( "    Calling msiObjStat" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiObjStat: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* parse inpParam1 */
    rei->status = parseMspForDataObjInp( inpParam1, &dataObjInp,
                                         &myDataObjInp, 0 );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiObjStat: input inpParam1 error. status = %d", rei->status );
        return rei->status;
    }

    rodsObjStat_t *rodsObjStatOut = NULL;
    rei->status = rsObjStat( rsComm, myDataObjInp, &rodsObjStatOut );

    if ( rei->status >= 0 ) {
        fillMsParam( outParam, NULL, RodsObjStat_MS_T, rodsObjStatOut, NULL );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiObjStat: rsObjStat failed for %s, status = %d",
                            myDataObjInp->objPath,
                            rei->status );
    }
    freeRodsObjStat( rodsObjStatOut );

    return rei->status;
}

/**
 * \fn msiDataObjRsync (msParam_t *inpParam1, msParam_t *inpParam2,
 *    msParam_t *inpParam3, msParam_t *inpParam4, msParam_t *outParam,
 *    ruleExecInfo_t *rei)
 *
 * \brief This microservice requests the client to call a rcDataObjRsync API
 *   as part of a workflow execution.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Michael Wan
 * \date    2007-02-12
 *
 * \note For now, this microservice should only be used for IRODS_TO_IRODS
 * mode because of the logistic difficulty with the microservice getting the
 * checksum values of the local file.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - A DataObjInp_MS_T or STR_MS_T which would be taken as dataObj path.
 * \param[in] inpParam2 - Optional - a STR_MS_T which specifies the rsync mode
 *      (RSYNC_MODE_KW). Valid mode is IRODS_TO_IRODS and IRODS_TO_COLLECTION.
 * \param[in] inpParam3 - Optional - a STR_MS_T which specifies the resource
  *      value (DEST_RESC_NAME_KW).
 * \param[in] inpParam4 - Optional - a STR_MS_T which specifies the
 *      (RSYNC_DEST_PATH_KW).  For IRODS_TO_IRODS, this is the target path.
 *       For IRODS_TO_COLLECTION, this is the top level target collection.
 *       e.g., if dataObj (inpParam1) is /tempZone/home/rods/foo and
 *       the target collection (inpParam4) is /tempZone/archive, then
 *       the target path is /tempZone/archive/home/rods/foo.
 * \param[out] outParam - a INT_MS_T containing the status.
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
msiDataObjRsync( msParam_t *inpParam1, msParam_t *inpParam2,
                 msParam_t *inpParam3, msParam_t *inpParam4, msParam_t *outParam,
                 ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    dataObjInp_t dataObjInp, *myDataObjInp = NULL;
    msParamArray_t *outParamArray = NULL;
    char *rsyncMode;
    char *targCollection, *tmpPtr;
    char targPath[MAX_NAME_LEN];

    RE_TEST_MACRO( "    Calling msiDataObjRsync" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjRsync: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* parse inpParam1 */
    rei->status = parseMspForDataObjInp( inpParam1, &dataObjInp,
                                         &myDataObjInp, 1 );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjRsync: input inpParam1 error. status = %d", rei->status );
        return rei->status;
    }

    if ( ( rei->status = parseMspForCondInp( inpParam2, &myDataObjInp->condInput,
                         RSYNC_MODE_KW ) ) < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjRsync: input inpParam2 error. status = %d", rei->status );
        return rei->status;
    }


    if ( ( rei->status = parseMspForCondInp( inpParam3, &myDataObjInp->condInput,
                         DEST_RESC_NAME_KW ) ) < 0 ) {
    }

    if ( ( rei->status = parseMspForCondInp( inpParam4, &myDataObjInp->condInput,
                         RSYNC_DEST_PATH_KW ) ) < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjRsync: input inpParam4 error. status = %d", rei->status );
        return rei->status;
    }

    /* just call rsDataObjRsync for now. client must supply the chksum of
     * the local file. Could ask the client to do a chksum first */

    rsyncMode = getValByKey( &myDataObjInp->condInput, RSYNC_MODE_KW );
    if ( rsyncMode == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjRsync: RSYNC_MODE_KW input is missing" );
        rei->status = USER_RSYNC_NO_MODE_INPUT_ERR;
        return rei->status;
    }

    if ( strcmp( rsyncMode, IRODS_TO_LOCAL ) == 0 ||
            strcmp( rsyncMode, LOCAL_TO_IRODS ) == 0 ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjRsync: local/iRODS rsync not supported for %s",
                 myDataObjInp->objPath );
        rei->status = NO_LOCAL_FILE_RSYNC_IN_MSI;
        return rei->status;
    }
    else if ( strcmp( rsyncMode, IRODS_TO_COLLECTION ) == 0 ) {
        targCollection = getValByKey( &myDataObjInp->condInput,
                                      RSYNC_DEST_PATH_KW );
        if ( targCollection == NULL ) {
            rodsLog( LOG_ERROR,
                     "msiDataObjRsync:  RSYNC_DEST_PATH_KW input for %s is missing",
                     myDataObjInp->objPath );
            rei->status = USER_INPUT_PATH_ERR;
            return rei->status;
        }
        tmpPtr = strchr( myDataObjInp->objPath + 1, '/' );
        if ( tmpPtr == NULL ) {
            rodsLog( LOG_ERROR,
                     "msiDataObjRsync:  problem parsing %s", myDataObjInp->objPath );
            rei->status = USER_INPUT_PATH_ERR;
            return rei->status;
        }
        snprintf( targPath, MAX_NAME_LEN, "%s%s", targCollection, tmpPtr );
        addKeyVal( &myDataObjInp->condInput, RSYNC_MODE_KW, IRODS_TO_IRODS );
        addKeyVal( &myDataObjInp->condInput, RSYNC_DEST_PATH_KW, targPath );
    }

    rei->status = rsDataObjRsync( rsComm, myDataObjInp, &outParamArray );

    if ( outParamArray != NULL ) {
        clearMsParamArray( outParamArray, 1 );
        free( outParamArray );
    }
    if ( myDataObjInp == &dataObjInp ) {
        clearKeyVal( &myDataObjInp->condInput );
    }

    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjRsync: rsDataObjRsync failed for %s, status = %d",
                            myDataObjInp->objPath,
                            rei->status );
    }

    return rei->status;
}

/**
 * \fn msiCollRsync (msParam_t *inpParam1, msParam_t *inpParam2,
 *    msParam_t *inpParam3, msParam_t *inpParam4, msParam_t *outParam,
 *    ruleExecInfo_t *rei)
 *
 * \brief This microservice recursively syncs a source
 *    collection to a target collection.
 *
 * \module core
 *
 * \since 2.4
 *
 * \author  Michael Wan
 * \date    2010-04-27
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - a STR_MS_T which specifies the source collection path.
 * \param[in] inpParam2 - a STR_MS_T which specifies the target collection path.
 * \param[in] inpParam3 - Optional - a STR_MS_T which specifies the target
 *      resource.
 * \param[in] inpParam4 - Optional - a STR_MS_T which specifies the rsync mode
 *      (RSYNC_MODE_KW). Valid mode is IRODS_TO_IRODS.
 * \param[out] outParam - a INT_MS_T containing the status.
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
msiCollRsync( msParam_t *inpParam1, msParam_t *inpParam2,
              msParam_t *inpParam3, msParam_t *inpParam4, msParam_t *outParam,
              ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    dataObjInp_t dataObjInp;
    char *rsyncMode;
    char *srcColl = NULL;
    char *destColl = NULL;

    RE_TEST_MACRO( "    Calling msiCollRsync" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiCollRsync: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    bzero( &dataObjInp, sizeof( dataObjInp ) );

    /* parse inpParam1 */
    srcColl = parseMspForStr( inpParam1 );

    if ( srcColl == NULL ) {
        rei->status = SYS_INVALID_FILE_PATH;
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiCollRsync: input inpParam1 error. status = %d", rei->status );
        return rei->status;
    }

    /* parse inpParam2 */
    destColl = parseMspForStr( inpParam2 );

    if ( destColl == NULL ) {
        rei->status = SYS_INVALID_FILE_PATH;
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiCollRsync: input inpParam2 error. status = %d", rei->status );
        return rei->status;
    }

    if ( ( rei->status = parseMspForCondInp( inpParam3, &dataObjInp.condInput,
                         DEST_RESC_NAME_KW ) ) < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiCollRsync: input inpParam3 error. status = %d", rei->status );
        return rei->status;
    }

    if ( ( rei->status = parseMspForCondInp( inpParam4, &dataObjInp.condInput,
                         RSYNC_MODE_KW ) ) < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiCollRsync: input inpParam4 error. status = %d", rei->status );
        return rei->status;
    }

    /* just call rsDataObjRsync for now. client must supply the chksum of
     * the local file. Could ask the client to do a chksum first */

    rsyncMode = getValByKey( &dataObjInp.condInput, RSYNC_MODE_KW );
    if ( rsyncMode == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiCollRsync: RSYNC_MODE_KW input is missing" );
        rei->status = USER_RSYNC_NO_MODE_INPUT_ERR;
        return rei->status;
    }

    if ( strcmp( rsyncMode, IRODS_TO_LOCAL ) == 0 ||
            strcmp( rsyncMode, LOCAL_TO_IRODS ) == 0 ) {
        rodsLog( LOG_ERROR,
                 "msiCollRsync: local/iRODS rsync not supported for %s",
                 srcColl );
        rei->status = NO_LOCAL_FILE_RSYNC_IN_MSI;
        return rei->status;
    }

    rei->status = _rsCollRsync( rsComm, &dataObjInp, srcColl, destColl );

    clearKeyVal( &dataObjInp.condInput );

    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiCollRsync: rsDataObjRsync failed for %s, status = %d",
                            srcColl,
                            rei->status );
    }

    return rei->status;
}

int
_rsCollRsync( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
              char *srcColl, char *destColl ) {
    collInp_t openCollInp;
    collEnt_t *collEnt;
    int handleInx;
    char parPath[MAX_NAME_LEN], childPath[MAX_NAME_LEN];
    char destChildPath[MAX_NAME_LEN];
    msParamArray_t *outParamArray = NULL;
    int status = 0;
    int status1 = 0;

    memset( &openCollInp, 0, sizeof( openCollInp ) );
    rstrcpy( openCollInp.collName, srcColl, MAX_NAME_LEN );
    openCollInp.flags = 0;
    handleInx = rsOpenCollection( rsComm, &openCollInp );
    if ( handleInx < 0 ) {
        rodsLog( LOG_ERROR,
                 "_rsCollRsync: rsOpenCollection of %s error. status = %d",
                 openCollInp.collName, handleInx );
        return handleInx;
    }

    rsMkCollR( rsComm, "/", destColl );
    while ( ( status1 = rsReadCollection( rsComm, &handleInx, &collEnt ) ) >= 0 ) {
        if ( collEnt->objType == DATA_OBJ_T ) {
            snprintf( dataObjInp->objPath, MAX_NAME_LEN, "%s/%s",
                      srcColl, collEnt->dataName );
            snprintf( destChildPath, MAX_NAME_LEN, "%s/%s",
                      destColl, collEnt->dataName );
            addKeyVal( &dataObjInp->condInput, RSYNC_DEST_PATH_KW,
                       destChildPath );
            status = rsDataObjRsync( rsComm, dataObjInp, &outParamArray );
            if ( outParamArray != NULL ) {
                clearMsParamArray( outParamArray, 1 );
                free( outParamArray );
            }
        }
        else if ( collEnt->objType == COLL_OBJ_T ) {
            if ( ( status = splitPathByKey(
                                collEnt->collName, parPath, MAX_NAME_LEN, childPath, MAX_NAME_LEN, '/' ) ) < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "_rsCollRsync:: splitPathByKey for %s error, status = %d",
                              collEnt->collName, status );
                freeCollEnt( collEnt );
                return status;
            }
            snprintf( destChildPath, MAX_NAME_LEN, "%s/%s",
                      destColl, childPath );

            status = _rsCollRsync( rsComm, dataObjInp, collEnt->collName,
                                   destChildPath );
        }
        free( collEnt );    /* just free collEnt but not content */
        if ( status < 0 ) {
            break;
        }
    }
    rsCloseCollection( rsComm, &handleInx );
    if ( status1 < 0 && status1 != CAT_NO_ROWS_FOUND ) {
        return status1;
    }
    else {
        return status;
    }
}

/**
 * \fn msiExecCmd (msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *inpParam3,
 * msParam_t *inpParam4, msParam_t *inpParam5, msParam_t *outParam,
 * ruleExecInfo_t *rei)
 *
 * \brief This microservice requests the client to call a rcExecCmd API
 *   to fork and execute a command that resides in the server/bin/cmd directory.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Michael Wan
 * \date    2007-05-08
 *
 * \note  This call does not require client interaction, which means
 *  it can be used through rcExecMyRule (irule) or internally by the server.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - a ExecCmd_MS_T or
 *    a STR_MS_T which specify the command (cmd) to execute.
 * \param[in] inpParam2 - Optional - a STR_MS_T which specifies the argv (cmdArgv)
 *    of the command
 * \param[in] inpParam3 - Optional - a STR_MS_T which specifies the host address
 *    (execAddr) to execute to command.
 * \param[in] inpParam4 - Optional - a STR_MS_T which specifies an iRODS file path
 *    (hintPath). The command will be executed on the host where this
 *    file is stored.
 * \param[in] inpParam5 - Optional - A INT_MS_T or a STR_MS_T. If it is greater
 *    than zero, the resolved physical path from the logical hintPath
 *    (inpParam4) will be used as the first argument the command.
 * \param[out] outParam - a ExecCmdOut_MS_T containing the status of the command
 *    execution and the stdout/strerr output.
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
msiExecCmd( msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *inpParam3,
            msParam_t *inpParam4, msParam_t *inpParam5, msParam_t *outParam,
            ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    execCmd_t execCmdInp, *myExecCmdInp;
    execCmdOut_t *execCmdOut = NULL;
    char *tmpPtr;

    RE_TEST_MACRO( "    Calling msiExecCmd" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiExecCmd: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* parse inpParam1 */
    rei->status = parseMspForExecCmdInp( inpParam1, &execCmdInp,
                                         &myExecCmdInp );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiExecCmd: input inpParam1 error. status = %d", rei->status );
        return rei->status;
    }

    if ( ( tmpPtr = parseMspForStr( inpParam2 ) ) != NULL ) {
        rstrcpy( myExecCmdInp->cmdArgv, tmpPtr, HUGE_NAME_LEN );
    }

    if ( ( tmpPtr = parseMspForStr( inpParam3 ) ) != NULL ) {
        rstrcpy( myExecCmdInp->execAddr, tmpPtr, LONG_NAME_LEN );
    }

    if ( ( tmpPtr = parseMspForStr( inpParam4 ) ) != NULL ) {
        rstrcpy( myExecCmdInp->hintPath, tmpPtr, MAX_NAME_LEN );
    }

    if ( parseMspForPosInt( inpParam5 ) > 0 ) {
        myExecCmdInp->addPathToArgv = 1;
    }

    if ( strlen( rei->ruleName ) > 0 &&
            strcmp( rei->ruleName, EXEC_MY_RULE_KW ) != 0 ) {
        /* does not come from rsExecMyRule */
        addKeyVal( &myExecCmdInp->condInput, EXEC_CMD_RULE_KW, rei->ruleName );
    }
    rei->status = rsExecCmd( rsComm, myExecCmdInp, &execCmdOut );

    if ( myExecCmdInp == &execCmdInp ) {
        clearKeyVal( &myExecCmdInp->condInput );
    }

    if ( execCmdOut != NULL ) {	/* something was written to it */
        fillMsParam( outParam, NULL, ExecCmdOut_MS_T, execCmdOut, NULL );
    }

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiExecCmd: rsExecCmd failed for %s, status = %d",
                            myExecCmdInp->cmd,
                            rei->status );
    }

    return rei->status;
}


/**
 * \fn msiCollRepl (msParam_t *collection, msParam_t *msKeyValStr, msParam_t *status,
 * ruleExecInfo_t *rei)
 *
 * \brief  This microservice wraps the rsCollRepl() routine to replicate a collection.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Antoine de Torcy
 * \date    2008-08-19
 *
 * \note  This call does not require client interaction, which means
 *  it can be used through rcExecMyRule (irule) or internally by the server.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] collection - A CollInp_MS_T or a STR_MS_T with the irods path of the
 *      collection to replicate.
 * \param[in] msKeyValStr - Optional - a STR_MS_T. This is the special msKeyValStr
 *      format of keyWd1=value1++++keyWd2=value2++++keyWd3=value3...
 *      If the keyWd is not specified (without the '=' char), the value is
 *      assumed to be the target resource ("destRescName") for backward
 *      compatibility.
 *      Valid keyWds are:
 *        \li "destRescName" - the target resource to replicate to.
 *        \li "backupRescName" - the target resource to backup
 *              the data. If this keyWd is used, the backup mode
 *              will be switched on.
 *        \li "rescName" - the resource of the source copy.
 *        \li "updateRepl" - update other replicas with the
 *              latest copy. This keyWd has no value. But
 *              the '=' character is still needed.
 *        \li "replNum" - the replica number to use as source.
 *        \li "numThreads" - the number of threads to use.
 *        \li "all" - replicate to all resources in the resource
 *              group. This keyWd has no value.
 *        \li "irodsAdmin" - admin user replicate other users' files.
 *              This keyWd has no value.
 *        \li "verifyChksum" - verify the transfer using checksum.
 *              This keyWd has no value.
 * \param[out] status - a CollOprStat_t for detailed operation status.
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
msiCollRepl( msParam_t *collection, msParam_t *msKeyValStr, msParam_t *status,
             ruleExecInfo_t *rei ) {
    /* for parsing collection input param */
    collInp_t collInpCache, *collInp;
    char *outBadKeyWd;
    int validKwFlags;

    /* misc. to avoid repeating rei->rsComm */
    rsComm_t *rsComm;


    /******************************  INIT *******************************/

    /* For testing mode when used with irule --test */
    RE_TEST_MACRO( "    Calling msiCollRepl" )

    /* Sanity checks */
    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR, "msiCollRepl: inp rei or rsComm is NULL." );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /**************************** PARAM PARSING  ************************/

    /* Parse target collection */
    rei->status =
        parseMspForCollInp( collection, &collInpCache, &collInp, 0 );

    if ( rei->status < 0 ) {
        rodsLog( LOG_ERROR,
                 "msiCollRepl: input collection error. status = %d", rei->status );
        return rei->status;
    }


    /* Parse resource name and directly write to collReplInp */
    validKwFlags = COLL_NAME_FLAG | DEST_RESC_NAME_FLAG |
                   BACKUP_RESC_NAME_FLAG | RESC_NAME_FLAG | UPDATE_REPL_FLAG |
                   REPL_NUM_FLAG | ALL_FLAG | ADMIN_FLAG | VERIFY_CHKSUM_FLAG;
    rei->status = parseMsKeyValStrForCollInp( msKeyValStr, collInp,
                  DEST_RESC_NAME_KW, validKwFlags, &outBadKeyWd );

    if ( rei->status < 0 ) {
        if ( outBadKeyWd != NULL ) {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiCollRepl: input keyWd - %s error. status = %d",
                                outBadKeyWd, rei->status );
            free( outBadKeyWd );
        }
        else {
            rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                "msiCollRepl: input msKeyValStr error. status = %d",
                                rei->status );
        }
        return rei->status;
    }

    /************************ API SERVER CALL **************************/

    /* Call rsCollRepl() */
    rei->status = rsCollRepl( rsComm, collInp, NULL );

    /*************************** OUTPUT PACKAGING ***********************/

    /* Send out op status */
    fillIntInMsParam( status, rei->status );

    return rei->status;
}

/**
 * \fn msiDataObjPutWithOptions (msParam_t *inpParam1, msParam_t *inpParam2,
 * msParam_t *inpParam3,msParam_t *inpOverwriteParam,
 * msParam_t *inpAllCopiesParam, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice requests the client to call a rcDataObjPut API
 *   as part of a workflow execution, with options.
 *
 * \deprecated Since 2.2, #msiDataObjPut can take a parameter with key-values format which make this microservice obsolete.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Romain Guinot
 * \date    2008
 *
 * \note  This microservice requests the client to call a rcDataObjPut API
 *   as part of a workflow execution, with options.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - a DataObjInp_MS_T or a STR_MS_T which would be taken as dataObj path.
 * \param[in] inpParam2 - Optional - a STR_MS_T which specifies the resource.
 * \param[in] inpParam3 - Optional - a STR_MS_T which specifies the client's local file path.
 * \param[in] inpOverwriteParam - Optional - a STR_MS_T which specifies if the put
 *      should do an overwrite if content already exists in the resource.
 *      To trigger an overwrite, "forceFlag" keyword is expected
 * \param[in] inpAllCopiesParam - Optional - a STR_MS_T which specifies if that
 *        in case of an overwrite,the operation should overwrite all existing copies
 * \param[out] outParam - a INT_MS_T containing the status.
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
msiDataObjPutWithOptions( msParam_t *inpParam1, msParam_t *inpParam2,
                          msParam_t *inpParam3, msParam_t *inpOverwriteParam,
                          msParam_t *inpAllCopiesParam, msParam_t *outParam, ruleExecInfo_t *rei ) {
    dataObjInp_t *myDataObjInp = NULL;

    RE_TEST_MACRO( "    Calling msiDataObjPut" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjPut: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm_t * rsComm = rei->rsComm;

    dataObjInp_t *dataObjInp = ( dataObjInp_t* )malloc( sizeof( *dataObjInp ) );
    /* parse inpParam1 */
    rei->status = parseMspForDataObjInp( inpParam1, dataObjInp,
                                         &myDataObjInp, 1 );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjPut: input inpParam1 error. status = %d", rei->status );
        clearDataObjInp( dataObjInp );
        free( dataObjInp );
        return rei->status;
    }

    rei->status = parseMspForCondInp( inpParam2, &dataObjInp->condInput,
                                      DEST_RESC_NAME_KW );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjPut: input inpParam2 error. status = %d", rei->status );
        clearDataObjInp( dataObjInp );
        free( dataObjInp );
        return rei->status;
    }


    rei->status = parseMspForCondInp( inpParam3, &dataObjInp->condInput,
                                      LOCAL_PATH_KW );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjPut: input inpParam3 error. status = %d", rei->status );
        clearDataObjInp( dataObjInp );
        free( dataObjInp );

        return rei->status;
    }

    if ( inpOverwriteParam != NULL &&
            strcmp( ( char* )inpOverwriteParam->inOutStruct, FORCE_FLAG_KW ) == 0 )
        rei->status = parseMspForCondInp( inpOverwriteParam,
                                          &dataObjInp->condInput, FORCE_FLAG_KW );

    if ( inpAllCopiesParam != NULL &&
            strcmp( ( char* )inpAllCopiesParam->inOutStruct, ALL_KW ) == 0 )
        rei->status = parseMspForCondInp( inpAllCopiesParam,
                                          &dataObjInp->condInput, ALL_KW );

    msParamArray_t * myMsParamArray = ( msParamArray_t* )malloc( sizeof( msParamArray_t ) );
    memset( myMsParamArray, 0, sizeof( msParamArray_t ) );

    rei->status = addMsParam( myMsParamArray, CL_PUT_ACTION, DataObjInp_MS_T,
                              ( void * ) dataObjInp, NULL );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjPut: addMsParam error. status = %d", rei->status );
        clearMsParamArray( myMsParamArray, 0 );
        free( myMsParamArray );
        clearDataObjInp( dataObjInp );
        free( dataObjInp );
        return rei->status;
    }

    /* tell the client to do the put */
    rei->status = sendAndRecvBranchMsg( rsComm, rsComm->apiInx,
                                        SYS_SVR_TO_CLI_MSI_REQUEST, ( void * ) myMsParamArray, NULL );

    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjPut: rsDataObjPut failed for %s, status = %d",
                            dataObjInp->objPath,
                            rei->status );
    }

    return rei->status;
}

/**
 * \fn msiDataObjReplWithOptions (msParam_t *inpParam1, msParam_t *inpParam2,
 *    msParam_t *inpParam3, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice is the same as msiDataObjRepl, but with more input options.
 *
 * \deprecated Since 2.2, #msiDataObjRepl can take a parameter with key-values format which make this microservice obsolete.
 *
 * \module core
 *
 * \since 2.1
 *
 * \author  Thomas Ledoux
 * \date    2009-03-24
 *
 * \note  Can be called by client through irule
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - a msParam of type DataObjInp_MS_T or STR_MS_T which would be the obj Path.
 * \param[in] inpParam2 - Optional - a STR_MS_T which specifies the resource.
 * \param[in] inpParam3 - Optional - a STR_MS_T which specifies an additional
 *     param like all (ALL_KW), irodsAdmin (ADMIN_KW)
 * \param[out] outParam - a INT_MS_T for the status.
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
msiDataObjReplWithOptions( msParam_t *inpParam1, msParam_t *inpParam2,
                           msParam_t *inpParam3, msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    dataObjInp_t dataObjInp, *myDataObjInp = NULL;
    transferStat_t *transStat = NULL;

    RE_TEST_MACRO( " Calling msiDataObjReplWithOptions" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjReplWithOptions: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* parse inpParam1 */
    rei->status = parseMspForDataObjInp( inpParam1, &dataObjInp,
                                         &myDataObjInp, 0 );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjReplWithOptions: input inpParam1 error. status = %d",
                            rei->status );
        return rei->status;
    }

    rei->status = parseMspForCondInp( inpParam2, &myDataObjInp->condInput,
                                      DEST_RESC_NAME_KW );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjReplWithOptions: input inpParam2 error. status = %d",
                            rei->status );
        return rei->status;
    }

    if ( ( rei->status = parseMspForCondKw( inpParam3, &myDataObjInp->condInput ) )
            < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjReplWithOptions: input inpParam3 error. status = %d",
                            rei->status );
        return rei->status;
    }

    rei->status = rsDataObjRepl( rsComm, myDataObjInp, &transStat );

    if ( myDataObjInp == &dataObjInp ) {
        clearKeyVal( &myDataObjInp->condInput );
    }

    if ( transStat != NULL ) {
        free( transStat );
    }

    if ( rei->status >= 0 ) {
        fillIntInMsParam( outParam, rei->status );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjReplWithOptions: rsDataObjRepl failed %s, status = %d",
                            myDataObjInp->objPath, rei->status );
    }

    return rei->status;
}

/**
 * \fn msiDataObjChksumWithOptions (msParam_t *inpParam1, msParam_t *inpParam2,
 *  msParam_t *inpParam3, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice calls rsDataObjChksum to checksum the iput data
 *    object as part of a workflow execution, with options.
 *
 * \deprecated Since 2.2, #msiDataObjChksum can take a parameter with key-values format which make this microservice obsolete.
 *
 * \module core
 *
 * \since 2.1
 *
 * \author  Thomas Ledoux
 * \date    2009-03-24
 *
 * \note  Can be called by client through irule
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - a msParam of type DataObjInp_MS_T or a STR_MS_T which would be taken as dataObj path.
 * \param[in] inpParam2 - Optional - a STR_MS_T which specifies "verifyChksum"
 *    (VERIFY_CHKSUM_KW) or "forceChksum"(FORCE_CHKSUM_KW).
 * \param[in] inpParam3 - Optional - a STR_MS_T which specifies the "ChksumAll"
 *   (CHKSUM_ALL_KW) or a INT which gives the replica number.
 * \param[out] outParam - a STR_MS_T containing the chksum value.
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
msiDataObjChksumWithOptions( msParam_t *inpParam1, msParam_t *inpParam2,
                             msParam_t *inpParam3, msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    dataObjInp_t dataObjInp, *myDataObjInp = NULL;
    char *chksum = NULL;

    RE_TEST_MACRO( " Calling msiDataObjChksumWithOptions" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiDataObjChksumRepl: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* parse inpParam1 */
    rei->status = parseMspForDataObjInp( inpParam1, &dataObjInp,
                                         &myDataObjInp, 1 );

    if ( rei->status < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjChksumWithOptions: input inpParam1 error. status = %d",
                            rei->status );
        return rei->status;
    }

    if ( ( rei->status = parseMspForCondKw( inpParam2,
                                            &myDataObjInp->condInput ) ) < 0 ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjChksumWithOptions: input inpParam2 error. status = %d",
                            rei->status );
        return rei->status;
    }

    if ( inpParam3 != NULL && strcmp( inpParam3->type, STR_MS_T ) == 0 ) {
        if ( strcmp( ( char * ) inpParam3->inOutStruct, CHKSUM_ALL_KW ) == 0 ) {
            if ( ( rei->status = parseMspForCondKw( inpParam3,
                                                    &myDataObjInp->condInput ) ) < 0 ) {
                rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                    "msiDataObjChksumWithOptions: input inpParam3 error.stat=%d",
                                    rei->status );
                return rei->status;
            }
        }
        else {
            /* replica number */
            if ( ( rei->status = parseMspForCondInp( inpParam3,
                                 &myDataObjInp->condInput, REPL_NUM_KW ) ) < 0 ) {
                rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                                    "msiDataObjChksumWithOptions: input inpParam3 error.stat=%d",
                                    rei->status );
                return rei->status;
            }
        }
    }
    rei->status = rsDataObjChksum( rsComm, myDataObjInp, &chksum );

    if ( myDataObjInp == &dataObjInp ) {
        clearKeyVal( &myDataObjInp->condInput );
    }

    if ( rei->status >= 0 ) {
        fillStrInMsParam( outParam, chksum );
        free( chksum );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDataObjChksumWithOptions: rsDataObjChksum failed for %s,stat=%d",
                            myDataObjInp->objPath, rei->status );
    }

    return rei->status;
}

/**
 * \fn msiTarFileExtract (msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *inpParam3,  msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief Extracts a tar object file into a target collection
 *
 * \module core
 *
 * \since 2.3
 *
 * \author  Jean-Yves Nief
 * \date    2009-06-15
 *
 * \note  This microservice calls rsStructFileExtAndReg to extract a tar
 *        file (inpParam1) into a target collection (inpParam2).  The content of
 *        the target collection is stored on the physical resource (inpParam3).
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - A StructFileExtAndRegInp_MS_T or
 *              a STR_MS_T which would be taken as dataObj path.
 * \param[in] inpParam2 - A STR_MS_T which specifies the target collection.
 * \param[in] inpParam3 - optional - A STR_MS_T which specifies the target resource.
 * \param[out] outParam - An INT_MS_T containing the status.
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
msiTarFileExtract( msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *inpParam3,  msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    structFileExtAndRegInp_t structFileExtAndRegInp,
                             *myStructFileExtAndRegInp;
    keyValPair_t regParam;
    modDataObjMeta_t modDataObjMetaInp;
    int rc1;
    char origDataType[NAME_LEN];

    RE_TEST_MACRO( " Calling msiTarFileExtract" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiTarFileExtract: input rei or rsComm is NULL" );
        if ( rei ) {
            rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        }
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* start building the structFileExtAndRegInp instance.
    extract from inpParam1 the tar file object path: tarFilePath
    and from inpParam2 the collection target: colTarget */

    if ( inpParam1 == NULL || inpParam2 == NULL ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiTarFileExtract: input Param1 and/or Param2 are NULL" );
        rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        return rei->status;
    }

    if ( strcmp( inpParam1->type, STR_MS_T ) == 0 ) {
        bzero( &structFileExtAndRegInp, sizeof( structFileExtAndRegInp ) );
        myStructFileExtAndRegInp = &structFileExtAndRegInp;
        snprintf( myStructFileExtAndRegInp->objPath, sizeof( myStructFileExtAndRegInp->objPath ),
                  "%s", ( char* )inpParam1->inOutStruct );
    }
    else if ( strcmp( inpParam1->type, StructFileExtAndRegInp_MS_T ) == 0 ) {
        myStructFileExtAndRegInp =
            ( structFileExtAndRegInp_t * ) inpParam1->inOutStruct;
    }
    else {
        rei->status = UNKNOWN_PARAM_IN_RULE_ERR;
        return rei->status;
    }

    if ( strcmp( inpParam2->type, STR_MS_T ) == 0 ) {
        if ( strcmp( ( char * ) inpParam2->inOutStruct, "null" ) != 0 ) {
            snprintf( myStructFileExtAndRegInp->collection, sizeof( myStructFileExtAndRegInp->collection ),
                      "%s", ( char* )inpParam2->inOutStruct );
        }
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiTarFileExtract: Unsupported input Param2 type %s",
                            inpParam2->type );
        rei->status = UNKNOWN_PARAM_IN_RULE_ERR;
        return rei->status;
    }

    if ( inpParam3 != NULL && strcmp( inpParam3->type, STR_MS_T ) == 0 &&
            strcmp( ( char * ) inpParam3->inOutStruct, "null" ) != 0 ) {
        addKeyVal( &myStructFileExtAndRegInp->condInput, DEST_RESC_NAME_KW,
                   ( char * ) inpParam3->inOutStruct );
        /* RESC_NAME_KW is needed so Open will pick this resource */
        addKeyVal( &myStructFileExtAndRegInp->condInput, RESC_NAME_KW,
                   ( char * ) inpParam3->inOutStruct );
    }

    if ( rei->doi != NULL ) { /* rei->doi may not exist */
        /* retrieve the input object data_type in order to rollback in case
        * of a tar extraction problem */
        snprintf( origDataType, sizeof( origDataType ), "%s", rei->doi->dataType );
        /* modify the input object data_type to "tar file" */
        memset( &regParam, 0, sizeof( regParam ) );
        addKeyVal( &regParam, DATA_TYPE_KW, "tar file" );
        modDataObjMetaInp.dataObjInfo = rei->doi;
        modDataObjMetaInp.regParam = &regParam;
        rc1 = rsModDataObjMeta( rsComm, &modDataObjMetaInp );
        if ( rc1 < 0 ) {
            irods::log( ERROR( rc1, "rsModDataObjMeta failed." ) );
        }
    }

    /* tar file extraction */
    rei->status = rsStructFileExtAndReg( rsComm, myStructFileExtAndRegInp );
    if ( rei->status < 0 && rei->doi != NULL ) {
        rodsLog( LOG_ERROR, "msiTarFileExtract: tar file extraction failed" );
        /* switch back to the original input object data_type as tar
        * extraction failed */
        memset( &regParam, 0, sizeof( regParam ) );
        addKeyVal( &regParam, DATA_TYPE_KW, origDataType );
        rc1 = rsModDataObjMeta( rsComm, &modDataObjMetaInp );
        if ( rc1 < 0 ) {
            irods::log( ERROR( rc1, "rsModDataObjMeta failed." ) );
        }
    }

    fillIntInMsParam( outParam, rei->status );

    return rei->status;
}

/**
 * \fn msiTarFileCreate (msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *inpParam3,  msParam_t *inpParam4, ruleExecInfo_t *rei)
 *
 * \brief Creates a tar object file from a target collection
 *
 * \module core
 *
 * \since 2.3
 *
 * \author  Jean-Yves Nief
 * \date    2009-06-15
 *
 * \note  This microservice calls rsStructFileBundle to create a tar file
 *        (inpParam1) from a target collection (inpParam2). The content of the
 *        target collection is stored on the physical resource (inpParam3).
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - A StructFileExtAndRegInp_MS_T or a STR_MS_T which would be taken as dataObj path.
 * \param[in] inpParam2 - A STR_MS_T which specifies the target collection.
 * \param[in] inpParam3 - optional - A STR_MS_T which specifies the target resource.
 * \param[out] inpParam4 - optional - A STR_MS_T which specifies if the force flag is set. Set it to "force" if
                                      the force option is needed, otherwise no force option will be used.
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
msiTarFileCreate( msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *inpParam3,  msParam_t *inpParam4, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    structFileExtAndRegInp_t structFileExtAndRegInp,
                             *myStructFileExtAndRegInp;

    RE_TEST_MACRO( " Calling msiTarFileCreate" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiTarFileCreate: input rei or rsComm is NULL" );
        if ( rei ) {
            rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        }
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* start building the structFileExtAndRegInp instance.
    extract from inpParam1 the tar file object path: tarFilePath
    and from inpParam2 the target collection: colTarget */

    if ( inpParam1 == NULL || inpParam2 == NULL ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiTarFileCreate: input Param1 and/or Param2 are NULL" );
        rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        return rei->status;
    }

    if ( strcmp( inpParam1->type, STR_MS_T ) == 0 ) {
        bzero( &structFileExtAndRegInp, sizeof( structFileExtAndRegInp ) );
        myStructFileExtAndRegInp = &structFileExtAndRegInp;
        snprintf( myStructFileExtAndRegInp->objPath, sizeof( myStructFileExtAndRegInp->objPath ),
                  "%s", ( char* )inpParam1->inOutStruct );
    }
    else if ( strcmp( inpParam1->type, StructFileExtAndRegInp_MS_T ) == 0 ) {
        myStructFileExtAndRegInp =
            ( structFileExtAndRegInp_t * ) inpParam1->inOutStruct;
    }
    else {
        rei->status = UNKNOWN_PARAM_IN_RULE_ERR;
        return rei->status;
    }

    if ( strcmp( inpParam2->type, STR_MS_T ) == 0 ) {
        if ( strcmp( ( char * ) inpParam2->inOutStruct, "null" ) != 0 ) {
            snprintf( myStructFileExtAndRegInp->collection, sizeof( myStructFileExtAndRegInp->collection ),
                      "%s", ( char* )inpParam2->inOutStruct );
        }
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiTarFileExtract: Unsupported input Param2 type %s",
                            inpParam2->type );
        rei->status = UNKNOWN_PARAM_IN_RULE_ERR;
        return rei->status;
    }

    if ( inpParam3 != NULL && strcmp( inpParam3->type, STR_MS_T ) == 0 &&
            strcmp( ( char * ) inpParam3->inOutStruct, "null" ) != 0 ) {
        addKeyVal( &myStructFileExtAndRegInp->condInput, DEST_RESC_NAME_KW,
                   ( char * ) inpParam3->inOutStruct );
    }

    if ( inpParam4 != NULL && strcmp( inpParam4->type, STR_MS_T ) == 0 &&
            strcmp( ( char * ) inpParam4->inOutStruct, "force" ) == 0 ) {
        addKeyVal( &myStructFileExtAndRegInp->condInput, FORCE_FLAG_KW, "" );
    }

    /* tar file extraction */
    rei->status = rsStructFileBundle( rsComm, myStructFileExtAndRegInp );

    return rei->status;

}



/**
 * \fn msiPhyBundleColl (msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief Bundles a collection into a number of tar files, similar to the iphybun command
 *
 * \module core
 *
 * \since 2.3
 *
 * \author  Jean-Yves Nief
 * \date    2009-06-15
 *
 * \note  This microservice calls rsPhyBundleColl to bundle files in a
 *        collection into a number of tar files to make it more efficient to
 *        store these files on tape. This microservice has the same functionality
 *        as the iphybun command.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - A StructFileExtAndRegInp_MS_T or a STR_MS_T which would be taken as the collection for the phybun.
 * \param[in] inpParam2 - optional - a STR_MS_T which specifies the target resource. If one wants to modify
 *                       the value of the maximum number of subfiles contained in the tar file (default=5120), the input should be like this:
 *                       "<target resource name>++++N=10000++++s=1". In this example, it will allow to merge up to 10000 files or up to
 *                        1 GB in volume, whatever occurs first, in a single tar file.
 *                        Note that if these numbers are too high (especially "N"), it can cause some significant overhead for
 *                        operations like retrieving a single file within a tar file (stage, untar and register in iRODS lots of files).
 *                        If the syntax after "++++" is invalid, it will be ignored.
 * \param[out] outParam - An INT_MS_T containing the status.
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
msiPhyBundleColl( msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;
    structFileExtAndRegInp_t structFileExtAndRegInp,
                             *myStructFileExtAndRegInp;

    RE_TEST_MACRO( " Calling msiPhyBundleColl" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiPhyBundleColl: input rei or rsComm is NULL" );
        if ( rei ) {
            rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        }
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    /* start building the structFileExtAndRegInp instance.
    extract from inpParam1 the tar file object path: tarFilePath
    and from inpParam2 the target collection: colTarget */


    if ( inpParam1 == NULL ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiPhyBundleColl: input Param1 is NULL" );
        rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        return rei->status;
    }

    // For consistency with iphybun
    if ( rei->uoic->authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        rei->status = SYS_NO_API_PRIV;
        rodsLog( LOG_ERROR, "msiPhyBundleColl: User %s does not have sufficient privilege, status = %d",
                 rei->uoic->userName, rei->status );
        return rei->status;
    }


    if ( strcmp( inpParam1->type, STR_MS_T ) == 0 ) {
        bzero( &structFileExtAndRegInp, sizeof( structFileExtAndRegInp ) );
        myStructFileExtAndRegInp = &structFileExtAndRegInp;
        snprintf( myStructFileExtAndRegInp->collection, sizeof( myStructFileExtAndRegInp->collection ),
                  "%s", ( char* )inpParam1->inOutStruct );

    }
    else if ( strcmp( inpParam1->type, StructFileExtAndRegInp_MS_T ) == 0 ) {
        myStructFileExtAndRegInp =
            ( structFileExtAndRegInp_t * ) inpParam1->inOutStruct;

    }
    else {

        rei->status = UNKNOWN_PARAM_IN_RULE_ERR;
        return rei->status;
    }

    if ( inpParam2 != NULL && strcmp( inpParam2->type, STR_MS_T ) == 0 &&
            strcmp( ( char * ) inpParam2->inOutStruct, "null" ) != 0 ) {
        /* parse the input parameter which is: <string> or <string>++++N=<int>.... */
        std::vector<std::string> tokens;
        boost::algorithm::split_regex( tokens, ( char * ) inpParam2->inOutStruct, boost::regex( "\\+\\+\\+\\+" ) );
        if ( !tokens[0].empty() ) {
            addKeyVal( &myStructFileExtAndRegInp->condInput, DEST_RESC_NAME_KW, tokens[0].c_str() );
        }
        for ( size_t i = 1; i < tokens.size(); i++ ) {
            if ( tokens[i].empty() ) {
                continue;
            }
            std::vector<std::string> current_arg;
            boost::algorithm::split_regex( current_arg, tokens[i], boost::regex( "=" ) );
            if ( current_arg.size() != 2 || current_arg[0].size() != 1 ) {
                rodsLog( LOG_ERROR, "msiPhyBundleColl called with improperly formatted arguments" );
                continue;
            }
            switch ( current_arg[0].c_str()[0] ) {
            case 'N':
                addKeyVal( &myStructFileExtAndRegInp->condInput, MAX_SUB_FILE_KW, current_arg[1].c_str() );
                break;
            case 'S':
                addKeyVal( &myStructFileExtAndRegInp->condInput, RESC_NAME_KW, current_arg[1].c_str() );
                break;
            case 's':
                addKeyVal( &myStructFileExtAndRegInp->condInput, MAX_BUNDLE_SIZE_KW, current_arg[1].c_str() );
                break;
            default:
                rodsLog( LOG_ERROR, "msiPhyBundleColl called with improperly formatted arguments" );
            }
        }
    }

    /* tar file extraction */
    rei->status = rsPhyBundleColl( rsComm, myStructFileExtAndRegInp );


    fillIntInMsParam( outParam, rei->status );


    return rei->status;

}

