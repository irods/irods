/**
 * @file        printMS.cpp
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
//#include "reGlobalsExtern.hpp"
#include "icatHighLevelRoutines.hpp"
#include "apiHeaderAll.h"
#include "rsApiHandler.hpp"
//#include "reFuncDefs.hpp"
#include "irods_re_structs.hpp"

#if defined(_LP64) || defined(__LP64__)
#define CAST_PTR_INT (long int)
#else
#define CAST_PTR_INT (uint)
#endif

/**
 * \cond oldruleengine
 * \fn writeLine(msParam_t* where, msParam_t* inString, ruleExecInfo_t *rei)
 *
 * \brief  This microservice writes a given string and newline character into the given buffer.
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \note   This microservice takes a given buffer string and appends it to the end of the buffer
 * (either stdout or stderr in ruleExecOut parameter) followed by a new line character.
 * This may be extended later for writing into local log file
 * or into an iRODS file also. The ruleExecOut is a system MS-parameter (*variable) that is automatically available.
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] where - a msParam of type STR_MS_T which is the buffer name in ruleExecOut. Currently stdout and stderr.
 * \param[in] inString - a msParam of type STR_MS_T which is a string to be written into buffer.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect ruleExecOut structure in msParamArray gets modified
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa writeString
 * \endcond
 **/
int writeLine( msParam_t* where, msParam_t* inString, ruleExecInfo_t *rei ) {
    int i;
    char tmp[3];
    char *ptr;
    char *writeId = ( char * ) where->inOutStruct;

    if ( writeId != NULL && strcmp( writeId, "serverLog" ) == 0 &&
            inString->inOutStruct != NULL ) {
        rodsLog( LOG_NOTICE, "writeLine: inString = %s\n", inString->inOutStruct );
        return 0;
    }

    i = writeString( where, inString, rei );
    if ( i < 0 ) {
        return i;
    }
    ptr = ( char* )inString->inOutStruct;
    sprintf( tmp, "%s\n", "" );
    inString->inOutStruct =  tmp;
    i = writeString( where, inString, rei );
    inString->inOutStruct = ptr;
    return i;

}

/**
 * \fn writePosInt(msParam_t* where, msParam_t* inInt, ruleExecInfo_t *rei)
 *
 * \brief  This microservice writes a positive integer into a buffer.
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] where - a msParam of type STR_MS_T which is the buffer name in ruleExecOut.
 * \param[in] inInt - the integer to write
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
int writePosInt( msParam_t* where, msParam_t* inInt, ruleExecInfo_t *rei ) {
    char *writeId;
    char writeStr[LONG_NAME_LEN];
    int status;

    if ( where->inOutStruct != NULL ) {
        writeId = ( char* )where->inOutStruct;
    }
    else {
        writeId = where->label;
    }

    if ( inInt->inOutStruct != NULL ) {
        sprintf( writeStr, "%d", parseMspForPosInt( inInt ) );
    }
    else {
        snprintf( writeStr, LONG_NAME_LEN, "%s", inInt->label );
    }

    status = _writeString( writeId, writeStr, rei );

    return status;
}


/**
 * \fn writeBytesBuf(msParam_t* where, msParam_t* inBuf, ruleExecInfo_t *rei)
 *
 * \brief  This microservice writes the buffer in an inOutStruct to stdout or stderr.
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] where - a msParam of type STR_MS_T which is the buffer name in ruleExecOut. It can be stdout or stderr.
 * \param[in] inBuf - a msParam of type BUF_LEN_MS_T.
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
int writeBytesBuf( msParam_t* where, msParam_t* inBuf, ruleExecInfo_t *rei ) {
    char *writeId;
    char *writeStr;
    int status;

    if ( where->inOutStruct != NULL ) {
        writeId = ( char* )where->inOutStruct;
    }
    else {
        writeId = where->label;
    }

    if ( inBuf->inpOutBuf ) {
        /* Buffer might no be null-terminated */
        writeStr = ( char* )malloc( inBuf->inpOutBuf->len + 1 );
        strncpy( writeStr, ( char* )inBuf->inpOutBuf->buf, inBuf->inpOutBuf->len );
        writeStr[inBuf->inpOutBuf->len] = '\0';
    }
    else {
        writeStr = strdup( inBuf->label );
    }

    status = _writeString( writeId, writeStr, rei );

    if ( writeStr != NULL ) {
        free( writeStr );
    }

    return status;
}

/**
 * \fn writeKeyValPairs(msParam_t *where, msParam_t *inKVPair, msParam_t *separator, ruleExecInfo_t *rei)
 *
 * \brief  This microservice writes keyword value pairs to stdout or stderr, using the given separator.
 *
 * \module core
 *
 * \since 2.1
 *
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] where - a msParam of type STR_MS_T which is the buffer name in ruleExecOut. It can be stdout or stderr.
 * \param[in] inKVPair - a msParam of type KeyValPair_MS_T
 * \param[in] separator - Optional - a msParam of type STR_MS_T, the desired parameter
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
int writeKeyValPairs( msParam_t *where, msParam_t *inKVPair, msParam_t *separator, ruleExecInfo_t *rei ) {
    keyValPair_t *KVPairs;
    char *writeId;
    char *writeStr;
    char *sepStr;
    int i;
    size_t size;


    RE_TEST_MACRO( "    Calling writeKeyValPairs" )


    /* sanity checks */
    if ( !rei ) {
        rodsLog( LOG_ERROR, "writeKeyValPairs: input rei is NULL." );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( !where ) {
        rodsLog( LOG_ERROR, "writeKeyValPairs: No destination provided for writing." );
        return USER__NULL_INPUT_ERR;
    }

    /* empty? */
    if ( !inKVPair || !inKVPair->inOutStruct ) {
        return 0;
    }

    /* check for proper input type and get keyValPair input */
    if ( inKVPair->type && strcmp( inKVPair->type, KeyValPair_MS_T ) ) {
        rodsLog( LOG_ERROR, "writeKeyValPairs: input parameter is not of KeyValPair_MS_T type." );
        return USER_PARAM_TYPE_ERR;
    }
    KVPairs = ( keyValPair_t * )inKVPair->inOutStruct;


    /* where are we writing to? */
    if ( where->inOutStruct != NULL ) {
        writeId = ( char* )where->inOutStruct;
    }
    else {
        writeId = where->label;
    }


    /* get separator string or use default */
    if ( ( sepStr = parseMspForStr( separator ) ) == NULL )  {
        sepStr = "\t|\t";
    }


    /* find out how much memory is needed for writeStr */
    size = 0;
    for ( i = 0; i < KVPairs->len; i++ ) {
        size += strlen( KVPairs->keyWord[i] ) + strlen( sepStr ) + strlen( KVPairs->value[i] ) + strlen( "\n" );
    }

    /* allocate memory for writeStr and pad with null chars */
    writeStr = ( char * )malloc( size + MAX_COND_LEN );
    memset( writeStr, '\0', size + MAX_COND_LEN );


    /* print each key-value pair to writeStr */
    for ( i = 0; i < KVPairs->len; i++ )  {
        strcat( writeStr, KVPairs->keyWord[i] );
        strcat( writeStr, sepStr );
        strcat( writeStr, KVPairs->value[i] );
        strcat( writeStr, "\n" );
    }


    /* call _writeString() routine */
    rei->status = _writeString( writeId, writeStr, rei );


    /* free writeStr since its content has been copied somewhere else */
    if ( writeStr != NULL ) {
        free( writeStr );
    }

    return rei->status;
}

