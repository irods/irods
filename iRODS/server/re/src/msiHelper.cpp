/**
 * @file  msiHelper.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* reHelper.c */

#include "reFuncDefs.hpp"
#include "msiHelper.hpp"
/**
 * \fn msiGetStdoutInExecCmdOut (msParam_t *inpExecCmdOut, msParam_t *outStr, ruleExecInfo_t *rei)
 *
 * \brief Gets stdout buffer from ExecCmdOut into buffer.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Mike Wan
 * \date  2009
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpExecCmdOut - a STR_MS_T which specifies the ExecCmdOut.
 * \param[out] outStr - a STR_MS_T to hold the retrieved stdout buffer.
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
msiGetStdoutInExecCmdOut( msParam_t *inpExecCmdOut, msParam_t *outStr,
                          ruleExecInfo_t *rei ) {
    char *strPtr;

    rei->status = getStdoutInExecCmdOut( inpExecCmdOut, &strPtr );

    if ( rei->status < 0 ) {
        return rei->status;
    }

    fillStrInMsParam( outStr, strPtr );
    free( strPtr );

    return rei->status;
}

/**
 * \fn msiGetStderrInExecCmdOut (msParam_t *inpExecCmdOut, msParam_t *outStr, ruleExecInfo_t *rei)
 *
 * \brief Gets stderr buffer from ExecCmdOut into buffer.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author Mike Wan
 * \date 2009
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpExecCmdOut - a STR_MS_T which specifies the ExecCmdOut.
 * \param[out] outStr - a STR_MS_T to hold the retrieved stderr buffer.
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
msiGetStderrInExecCmdOut( msParam_t *inpExecCmdOut, msParam_t *outStr,
                          ruleExecInfo_t *rei ) {
    char *strPtr = NULL;

    rei->status = getStderrInExecCmdOut( inpExecCmdOut, &strPtr );

    if ( rei->status < 0 ) {
        if ( rei->status != SYS_INTERNAL_NULL_INPUT_ERR ) {
            return rei->status;
        }
        else {
            free( strPtr );
            strPtr = NULL;
            rei->status = 0;
        }
    }

    if ( strPtr != NULL ) {
        fillStrInMsParam( outStr, strPtr );
        free( strPtr );
    }
    else {
        fillStrInMsParam( outStr, "" );
    }

    return rei->status;
}

/**
 * \fn msiWriteRodsLog (msParam_t *inpParam1,  msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief Writes a message into the server rodsLog.
 *
 * \module core
 *
 * \since 2.3
 *
 * \author  Jean-Yves Nief
 * \date    2009-06-15
 *
 * \note  This call should only be used through the rcExecMyRule (irule) call
 *        i.e., rule execution initiated by clients and should not be called
 *        internally by the server since it interacts with the client through
 *        the normal client/server socket connection.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - A STR_MS_T which specifies the message to log.
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
msiWriteRodsLog( msParam_t *inpParam1,  msParam_t *outParam, ruleExecInfo_t *rei ) {
    rsComm_t *rsComm;

    RE_TEST_MACRO( " Calling msiWriteRodsLog" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiWriteRodsLog: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    if ( inpParam1 == NULL ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiWriteRodsLog: input Param1 is NULL" );
        rei->status = USER__NULL_INPUT_ERR;
        return rei->status;
    }

    if ( strcmp( inpParam1->type, STR_MS_T ) == 0 ) {
        rodsLog( LOG_NOTICE,
                 "msiWriteRodsLog message: %s", inpParam1->inOutStruct );
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiWriteRodsLog: Unsupported input Param1 types %s",
                            inpParam1->type );
        rei->status = UNKNOWN_PARAM_IN_RULE_ERR;
        return rei->status;
    }

    rei->status = 0;

    fillIntInMsParam( outParam, rei->status );

    return rei->status;
}

/**
 * \fn msiAddKeyValToMspStr (msParam_t *keyStr, msParam_t *valStr, msParam_t *msKeyValStr, ruleExecInfo_t *rei)
 *
 * \brief Adds a key and value to existing msKeyValStr which is a  special
 * kind of STR_MS_T which has the format -
 * keyWd1=value1++++keyWd2=value2++++keyWd3=value3...
 *
 * \module core
 *
 * \since since 2.3
 *
 * \author Mike Wan
 * \date   2010
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] keyStr - a STR_MS_T key to be added to msKeyValStr.
 * \param[in] valStr - a STR_MS_T value to be added to msKeyValStr.
 * \param[in] msKeyValStr - a msKeyValStr to hold the new keyVal pair.
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
msiAddKeyValToMspStr( msParam_t *keyStr, msParam_t *valStr,
                      msParam_t *msKeyValStr, ruleExecInfo_t *rei ) {
    RE_TEST_MACRO( " Calling msiAddKeyValToMspStr" )

    if ( rei == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiAddKeyValToMspStr: input rei is NULL" );
        // JMC cppcheck rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        return SYS_INTERNAL_NULL_INPUT_ERR;//(rei->status);
    }

    rei->status = addKeyValToMspStr( keyStr, valStr, msKeyValStr );


    return rei->status;
}

/**
 * \fn msiSplitPath (msParam_t *inpPath,  msParam_t *outParentColl, msParam_t *outChildName, ruleExecInfo_t *rei)
 *
 * \brief Splits a pathname into parent and child values.
 *
 * \module core
 *
 * \since 2.3
 *
 * \author Mike Wan
 * \date   2010
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpPath - a STR_MS_T which specifies the pathname to split.
 * \param[out] outParentColl - a STR_MS_T to hold the returned parent path.
 * \param[out] outChildName - a STR_MS_T to hold the returned child value.
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
msiSplitPath( msParam_t *inpPath,  msParam_t *outParentColl,
              msParam_t *outChildName, ruleExecInfo_t *rei ) {
    char parent[MAX_NAME_LEN], child[MAX_NAME_LEN];

    RE_TEST_MACRO( " Calling msiSplitPath" )

    if ( rei == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiSplitPath: input rei is NULL" );
        // JMC cppcheck - rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        return SYS_INTERNAL_NULL_INPUT_ERR;//(rei->status);
    }

    if ( inpPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiSplitPath: input inpPath is NULL" );
        rei->status = USER__NULL_INPUT_ERR;
        return rei->status;
    }

    if ( strcmp( inpPath->type, STR_MS_T ) == 0 ) {
        if ( ( rei->status = splitPathByKey( ( char * ) inpPath->inOutStruct,
                                             parent, MAX_NAME_LEN, child, MAX_NAME_LEN, '/' ) ) < 0 ) {
            rodsLog( LOG_ERROR,
                     "msiSplitPath: splitPathByKey for %s error, status = %d",
                     ( char * ) inpPath->inOutStruct, rei->status );
        }
        else {
            fillStrInMsParam( outParentColl, parent );
            fillStrInMsParam( outChildName, child );
        }
    }
    else {
        rodsLog( LOG_ERROR,
                 "msiSplitPath: Unsupported input inpPath types %s",
                 inpPath->type );
        rei->status = UNKNOWN_PARAM_IN_RULE_ERR;
    }
    return rei->status;
}

/**
 * \fn msiGetSessionVarValue (msParam_t *inpVar,  msParam_t *outputMode, ruleExecInfo_t *rei)
 *
 * \brief Gets the value of a session variable in the rei
 *
 * \module core
 *
 * \since 2.3
 *
 * \author  Michael Wan
 * \date    2009-06-15
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpVar - A STR_MS_T which specifies the name of the session
 *             variable to output. The input session variable should NOT start
 *             with the "$" character. An input value of "all" means
 *             output all valid session variables.
 * \param[in] outputMode - A STR_MS_T which specifies the output mode. Valid modes are:
 *      \li "server" - log the output to the server log
 *      \li "client" - send the output to the client in rError
 *      \li "all" - both client and server
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
msiGetSessionVarValue( msParam_t *inpVar,  msParam_t *outputMode, ruleExecInfo_t *rei ) {
    char *inpVarStr, *outputModeStr;
    char errMsg[ERR_MSG_LEN];
    rsComm_t *rsComm;

    RE_TEST_MACRO( " Calling msiGetSessionVarValue" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiGetSessionVar: input rei or rei->rsComm is NULL" );
        // JMC cppcheck - rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        return SYS_INTERNAL_NULL_INPUT_ERR;//(rei->status);
    }

    if ( inpVar == NULL || outputMode == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiGetSessionVarValue: input inpVar or outputMode is NULL" );
        rei->status = USER__NULL_INPUT_ERR;
        return rei->status;
    }

    if ( strcmp( inpVar->type, STR_MS_T ) != 0 ||
            strcmp( outputMode->type, STR_MS_T ) != 0 ) {
        rodsLog( LOG_ERROR,
                 "msiGetSessionVarValue: Unsupported *inpVar or outputMode type" );
        rei->status = UNKNOWN_PARAM_IN_RULE_ERR;
        return rei->status;
    }
    rsComm = rei->rsComm;
    inpVarStr = ( char * ) inpVar->inOutStruct;
    outputModeStr = ( char * ) outputMode->inOutStruct;

    if ( inpVarStr == NULL || outputModeStr == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiGetSessionVarValue: input inpVar or outputMode is NULL" );
        rei->status = USER__NULL_INPUT_ERR;
        return rei->status;
    }

    if ( strcmp( inpVarStr, "all" ) == 0 ) {
        keyValPair_t varKeyVal;
        int i;
        bzero( &varKeyVal, sizeof( varKeyVal ) );
        rei->status = getAllSessionVarValue( rei, &varKeyVal );
        if ( rei->status >= 0 ) {
            if ( strcmp( outputModeStr, "server" ) == 0 ||
                    strcmp( outputModeStr, "all" ) == 0 ) {
                for ( i = 0; i < varKeyVal.len; i++ ) {
                    printf( "msiGetSessionVar: %s=%s\n",
                            varKeyVal.keyWord[i], varKeyVal.value[i] );
                }
            }
            if ( strcmp( outputModeStr, "client" ) == 0 ||
                    strcmp( outputModeStr, "all" ) == 0 ) {
                for ( i = 0; i < varKeyVal.len; i++ ) {
                    snprintf( errMsg, ERR_MSG_LEN,
                              "msiGetSessionVarValue: %s=%s\n",
                              varKeyVal.keyWord[i], varKeyVal.value[i] );
                    addRErrorMsg( &rsComm->rError, 0, errMsg );
                }
            }
            clearKeyVal( &varKeyVal );
        }
    }
    else {
        char *outStr = NULL;
        rei->status = getSessionVarValue( "", inpVarStr, rei, &outStr );
        if ( rei->status >= 0 && outStr ) { // cppcheck - Possible null pointer dereference: outStr
            if ( strcmp( outputModeStr, "server" ) == 0 ||
                    strcmp( outputModeStr, "all" ) == 0 ) {
                if ( NULL != outStr ) { // JMC cppcheck
                    printf( "msiGetSessionVarValue: %s=%s\n", inpVarStr, outStr );
                }
            }
            if ( strcmp( outputModeStr, "client" ) == 0 ||
                    strcmp( outputModeStr, "all" ) == 0 ) {
                snprintf( errMsg, ERR_MSG_LEN,
                          "msiGetSessionVarValue: %s=%s\n", inpVarStr, outStr );
                addRErrorMsg( &rsComm->rError, 0, errMsg );
            }
        }
        if ( outStr != NULL ) {
            free( outStr );
        }
    }
    return rei->status;
}

/**
 * \fn msiStrlen (msParam_t *stringIn,  msParam_t *lengthOut, ruleExecInfo_t *rei)
 *
 * \brief Returns the length of a given string.
 *
 * \module core
 *
 * \since after 2.4
 *
 * \author Mike Wan
 * \date   2010
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] stringIn - a STR_MS_T which specifies the input string.
 * \param[out] lengthOut - a STR_MS_T to hold the returned string length.
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
 * \retval the length of the given string
 * \pre none
 * \post none
 * \sa none
**/
int
msiStrlen( msParam_t *stringIn,  msParam_t *lengthOut, ruleExecInfo_t *rei ) {
    char len[NAME_LEN];

    RE_TEST_MACRO( " Calling msiStrlen" )

    if ( rei == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiStrlen: input rei is NULL" );
        // JMC cppcheck - rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        return SYS_INTERNAL_NULL_INPUT_ERR;//(rei->status);
    }

    if ( stringIn == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiStrlen: input stringIn is NULL" );
        rei->status = USER__NULL_INPUT_ERR;
        return rei->status;
    }

    if ( strcmp( stringIn->type, STR_MS_T ) == 0 ) {
        if ( stringIn->inOutStruct != NULL ) {
            rei->status = strlen( ( char * )  stringIn->inOutStruct );
        }
        else {
            rei->status = 0;
        }
    }
    else {
        rodsLog( LOG_ERROR,
                 "msiStrlen: Unsupported input stringIn types %s",
                 stringIn->type );
        rei->status = UNKNOWN_PARAM_IN_RULE_ERR;
    }
    snprintf( len, NAME_LEN, "%d", rei->status );
    fillStrInMsParam( lengthOut, len );
    return rei->status;
}

/**
 * \fn msiStrchop (msParam_t *stringIn,  msParam_t *stringOut, ruleExecInfo_t *rei)
 *
 * \brief Removes the last character of a given string.
 *
 * \module core
 *
 * \since after 2.4
 *
 * \author Mike Wan
 * \date   2010
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] stringIn - a STR_MS_T which specifies the input string.
 * \param[out] stringOut - a STR_MS_T to hold the string without the last char.
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
 * \retval the length of the returned string
 * \pre none
 * \post none
 * \sa none
**/
int
msiStrchop( msParam_t *stringIn,  msParam_t *stringOut, ruleExecInfo_t *rei ) {
    RE_TEST_MACRO( " Calling msiStrchop" )

    if ( rei == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiStrchop: input rei is NULL" );
        // JMC cppcheck - rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        return SYS_INTERNAL_NULL_INPUT_ERR;//(rei->status);
    }

    if ( stringIn == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiStrchop: input stringIn is NULL" );
        rei->status = USER__NULL_INPUT_ERR;
        return rei->status;
    }

    if ( strcmp( stringIn->type, STR_MS_T ) == 0 ) {
        if ( stringIn->inOutStruct != NULL ) {
            fillStrInMsParam( stringOut, ( char * ) stringIn->inOutStruct );
            rei->status = strlen( ( char * )  stringIn->inOutStruct );
            if ( rei->status > 0 ) {
                rei->status --;
                *( ( char * ) stringOut->inOutStruct + rei->status ) = '\0';
            }
        }
        else {
            fillStrInMsParam( stringOut, "" );
            rei->status = 0;
        }
    }
    else {
        rodsLog( LOG_ERROR,
                 "msiStrchop: Unsupported input stringIn types %s",
                 stringIn->type );
        rei->status = UNKNOWN_PARAM_IN_RULE_ERR;
    }
    return rei->status;
}

/**
 * \fn msiSubstr (msParam_t *stringIn,  msParam_t *offset, msParam_t *length, msParam_t *stringOut, ruleExecInfo_t *rei)
 *
 * \brief Returns a substring of the given string.
 *
 * \module core
 *
 * \since after 2.4
 *
 * \author Mike Wan
 * \date   2010
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] stringIn - a STR_MS_T which specifies the input string.
 * \param[in] offset - a STR_MS_T which specifies the position of the
 *    beginning of the substring (0 is first character). If negative, then
 *    offset specifies the position from the end of the string
 *    (-1 is the last character).
 * \param[in] length - a STR_MS_T which specifies the length of substring to
 *    return. If length is not specified, too large, negative, or "null",
 *    then return the substring from the offset to the end of stringIn.
 * \param[out] stringOut - a STR_MS_T to hold the resulting substring.
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
 * \retval the length of the substring
 * \pre none
 * \post none
 * \sa none
**/
int
msiSubstr( msParam_t *stringIn,  msParam_t *offset, msParam_t *length,
           msParam_t *stringOut, ruleExecInfo_t *rei ) {
    char *origStr, *strPtr;
    int intLength, intOffset;
    int origLen;
    char savedChar;
    char *savedPtr = NULL;

    RE_TEST_MACRO( " Calling msiSubstr" )

    if ( rei == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiSubstr: input rei is NULL" );
        // JMC cppcheck - rei->status = SYS_INTERNAL_NULL_INPUT_ERR;
        return SYS_INTERNAL_NULL_INPUT_ERR;// (rei->status);
    }

    if ( stringIn == NULL || offset == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiSubstr: input stringIn or offset is NULL" );
        rei->status = USER__NULL_INPUT_ERR;
        return rei->status;
    }

    if ( strcmp( stringIn->type, STR_MS_T ) != 0 ) {
        rodsLog( LOG_ERROR,
                 "msiSubstr: Unsupported input stringIn types %s",
                 stringIn->type );
        rei->status = UNKNOWN_PARAM_IN_RULE_ERR;
        return rei->status;
    }
    else {
        origStr = ( char * ) stringIn->inOutStruct;
    }

    if ( strcmp( offset->type, STR_MS_T ) != 0 ) {
        rodsLog( LOG_ERROR,
                 "msiSubstr: Unsupported input offset types %s",
                 offset->type );
        rei->status = UNKNOWN_PARAM_IN_RULE_ERR;
        return rei->status;
    }
    else {
        intOffset = atoi( ( char * ) offset->inOutStruct );
    }

    if ( length == NULL ) {
        /* not defined */
        intLength = -1;
    }
    else if ( strcmp( length->type, STR_MS_T ) != 0 ) {
        rodsLog( LOG_ERROR,
                 "msiSubstr: Unsupported input length types %s",
                 length->type );
        rei->status = UNKNOWN_PARAM_IN_RULE_ERR;
        return rei->status;
    }
    else if ( strcmp( ( char * ) length->inOutStruct, "null" ) == 0 ) {
        intLength = -1;
    }
    else {
        intLength = atoi( ( char * ) length->inOutStruct );
    }

    if ( intOffset >= 0 ) {
        strPtr = origStr + intOffset;
    }
    else {
        /* from the end. -1 is the last char */
        origLen = strlen( origStr );
        strPtr = origStr + origLen + intOffset;
    }

    if ( intLength >= 0 && ( ( int ) strlen( strPtr ) ) > intLength ) {
        /* put a null at the end of the sub str */
        savedPtr = strPtr + intLength;
        savedChar = *savedPtr;
        *savedPtr = '\0';
    }

    fillStrInMsParam( stringOut, strPtr );
    if ( savedPtr != NULL ) {
        /* restore */
        *savedPtr = savedChar;
    }

    rei->status = strlen( ( char * ) stringOut->inOutStruct );
    return rei->status;
}

/**
 * \fn msiExit (msParam_t *inpParam1, msParam_t *inpParam2, ruleExecInfo_t *rei)
 *
 * \brief Add a user message to the error stack.
 *
 * \module core
 *
 * \since after 2.4.1
 *
 * \author  Jean-Yves Nief
 * \date    2010-11-22
 *
 * \note  This call should only be used through the rcExecMyRule (irule) call
 *        i.e., rule execution initiated by clients and should not be called
 *        internally by the server since it interacts with the client through
 *        the normal client/server socket connection.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - A STR_MS_T which specifies the status error to add to the error stack.
 * \param[in] inpParam2 - A STR_MS_T which specifies the message to add to the error stack.
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
msiExit( msParam_t *inpParam1, msParam_t *inpParam2, ruleExecInfo_t *rei ) {
    char errMsg[ERR_MSG_LEN];
    int status;
    rsComm_t *rsComm;

    RE_TEST_MACRO( " Calling msiExit" )

    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiExit: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rsComm = rei->rsComm;

    if ( inpParam1 == NULL ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiExit: input Param1 is NULL" );
        rei->status = USER__NULL_INPUT_ERR;
        return rei->status;
    }

    if ( inpParam2 == NULL ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiExit: input Param2 is NULL" );
        rei->status = USER__NULL_INPUT_ERR;
        return rei->status;
    }

    if ( strcmp( inpParam1->type, STR_MS_T ) == 0 && strcmp( inpParam2->type, STR_MS_T ) == 0 ) {
        snprintf( errMsg, ERR_MSG_LEN, "%s\n", ( char * ) inpParam2->inOutStruct );
        status = atoi( ( char * ) inpParam1->inOutStruct );
        addRErrorMsg( &rsComm->rError, status, errMsg );
        return status;
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiExit: Unsupported input Param1 types %s",
                            inpParam1->type );
        rei->status = UNKNOWN_PARAM_IN_RULE_ERR;
        return rei->status;
    }

    /* return (rei->status); */
}

int
msiStrCat( msParam_t *targParam, msParam_t *srcParam, ruleExecInfo_t *rei ) {
    char *targ, *src, *newTarg;
    int targLen, srcLen;

    RE_TEST_MACRO( "    Calling msiStrCat" )

    if ( targParam == NULL || srcParam == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    if ( strcmp( targParam->type, STR_MS_T ) != 0 ||
            strcmp( srcParam->type, STR_MS_T ) != 0 ) {
        rodsLog( LOG_ERROR,
                 "msiStrCat: targParam and srcParam must be STR_MS_T. targ %s, src %s",
                 targParam->type, srcParam->type );
        return USER_PARAM_TYPE_ERR;
    }
    else {
        targ = ( char* ) targParam->inOutStruct;
        src = ( char* ) srcParam->inOutStruct;
    }

    targLen = strlen( targ );
    srcLen = strlen( src );
    newTarg = ( char * ) calloc( 1, targLen + srcLen + 10 );
    if ( targLen > 0 ) {
        rstrcpy( newTarg, targ, targLen + 1 );
    }
    if ( srcLen > 0 ) {
        rstrcpy( newTarg + targLen, src, srcLen + 1 );
    }
    free( targParam->inOutStruct );
    targParam->inOutStruct = newTarg;

    return 0;
}
int
msiSplitPathByKey( msParam_t *inpPath,  msParam_t *inpKey, msParam_t *outParentColl,
                   msParam_t *outChildName, ruleExecInfo_t *rei ) {
    char parent[MAX_NAME_LEN], child[MAX_NAME_LEN];

    RE_TEST_MACRO( " Calling msiSplitPathByKey" )

    if ( rei == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiSplitPathByKey: input rei is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( inpPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiSplitPathByKey: input inpPath is NULL" );
        rei->status = USER__NULL_INPUT_ERR;
        return rei->status;
    }

    if ( strcmp( inpPath->type, STR_MS_T ) == 0 ) {
        if ( ( rei->status = splitPathByKey( ( char * ) inpPath->inOutStruct,
                                             parent, MAX_NAME_LEN, child, MAX_NAME_LEN, *( char * ) inpKey->inOutStruct ) ) < 0 ) {
            rodsLog( LOG_ERROR,
                     "msiSplitPathByKey: splitPathByKey for %s error, status = %d",
                     ( char * ) inpPath->inOutStruct, rei->status );
        }
        else {
            fillStrInMsParam( outParentColl, parent );
            fillStrInMsParam( outChildName, child );
        }
    }
    else {
        rodsLog( LOG_ERROR,
                 "msiSplitPathByKey: Unsupported input inpPath types %s",
                 inpPath->type );
        rei->status = UNKNOWN_PARAM_IN_RULE_ERR;
    }
    return rei->status;
}


