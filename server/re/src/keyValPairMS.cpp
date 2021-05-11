#include "icatHighLevelRoutines.hpp"
#include "msParam.h"
#include "rcMisc.h"
#include "irods_re_structs.hpp"
#include "rodsErrorTable.h"

/**
 * \brief Given a list of KVPairs and a Key, this microservice gets the corresponding value.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] _kvp        This msParam is of type KeyValPair_PI which is a KeyValPair List.
 * \param[in] _key        This msParam is of type STR_MS_T which is a key.
 * \param[out] _out_value This msParam is of type STR_MS_T which is a value corresponding to key.
 * \param[in,out] _rei    \parblock The RuleExecInfo structure that is automatically handled by the rule engine.
 * The user does not include rei as a parameter in the rule invocation.
 * \endparblock
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
int msiGetValByKey(msParam_t* _kvp, msParam_t* _key, msParam_t* _out_value, ruleExecInfo_t* _rei)
{
    RE_TEST_MACRO("msiGetValByKey");

    if (!_kvp || !_key) {
        rodsLog(LOG_ERROR, "Missing one or more input arguments.");
        return SYS_INVALID_INPUT_PARAM;
    }

    if (std::strcmp(KeyValPair_MS_T, _kvp->type) != 0 ||
        std::strcmp(STR_MS_T, _key->type) != 0)
    {
        rodsLog(LOG_ERROR, "Incorrect type for one or more input arguments.");
        return SYS_INVALID_INPUT_PARAM;
    }

    const auto* kvp = static_cast<KeyValPair*>(_kvp->inOutStruct);
    const auto* key = static_cast<char*>(_key->inOutStruct);
 
    if (!key) {
        key = _key->label;

        if (!key) {
            rodsLog(LOG_ERROR, "Lookup key is null.");
            return SYS_INVALID_INPUT_PARAM;
        }
    }

    auto* value = getValByKey(kvp, key);

    if (!value) {
        rodsLog(LOG_ERROR, "Lookup key [%s] does not map to any value.", key);
        return UNMATCHED_KEY_OR_INDEX;
    }

    fillStrInMsParam(_out_value, value);

    return 0;
}

/**
 * \fn msiPrintKeyValPair(msParam_t* where, msParam_t* inkvpair, ruleExecInfo_t *rei)
 *
 * \brief  Prints key-value pairs to rei's stdout separated by " = "
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \note #msiExecStrCondQuery is used to
 *         run the query:  "SELECT DATA_NAME, DATA_REPL_NUM, DATA_CHECKSUM WHERE DATA_NAME LIKE 'foo%'".
 *         The result is printed using #msiPrintKeyValPair, which prints each row as an attribute-value pair.
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] where - a msParam of type STR_MS_T which is either stderr or stdout.
 * \param[in] inkvpair - a msParam of type KeyValPair_PI which is a KeyValPair list (structure).
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
int msiPrintKeyValPair( msParam_t* where, msParam_t* inkvpair, ruleExecInfo_t *rei ) {
    int i, l, m, j;
    keyValPair_t *k;
    char *s;
    msParam_t tms;

    RE_TEST_MACRO( "msiPrintKeyValPair" );

    m = 0;
    s = NULL;
    k = ( keyValPair_t* )inkvpair->inOutStruct;

    for ( i = 0; i < k->len; i++ ) {
        l  =  strlen( k->keyWord[i] ) + strlen( k->value[i] ) + 10;
        if ( l > m ) {
            if ( m > 0 ) {
                free( s );
            }
            s = ( char* )malloc( l );
            m = l;
        }
        sprintf( s, "%s = %s\n", k->keyWord[i], k->value[i] );
        tms.inOutStruct =  s;
        j = writeString( where, &tms, rei );
        if ( j < 0 ) {
            free( s );
            return j;
        }
    }
    if ( m > 0 ) {
        free( s );
    }
    return 0;
}

/**
 * \fn msiString2KeyValPair(msParam_t *inBufferP, msParam_t* outKeyValPairP, ruleExecInfo_t *rei)
 *
 * \brief  This microservice converts a %-separated key=value pair strings into keyValPair structure.
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] inBufferP - a msParam of type STR_MS_T which is key=value pairs separated by %-sign.
 * \param[out] outKeyValPairP - a msParam of type KeyValPair_MS_T which is a keyValuePair structure.
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
 * \sa msiStrArray2String
**/
int msiString2KeyValPair( msParam_t *inBufferP, msParam_t* outKeyValPairP, ruleExecInfo_t *rei ) {
    keyValPair_t *kvp;
    strArray_t strArray;
    char *buf;
    char *value;
    int i, j;
    char *valPtr;
    char *tmpPtr;

    RE_TEST_MACRO( "msiString2KeyValPair" );

    buf = strdup( ( char * )  inBufferP->inOutStruct );
    memset( &strArray, 0, sizeof( strArray_t ) );
    i =  parseMultiStr( buf, &strArray );
    free( buf );
    if ( i < 0 ) {
        return i;
    }
    value = strArray.value;

    kvp = ( keyValPair_t* )malloc( sizeof( keyValPair_t ) );
    memset(kvp, 0, sizeof(keyValPair_t));
    for ( i = 0; i < strArray.len; i++ ) {
        valPtr = &value[i * strArray.size];
        if ( ( tmpPtr = strstr( valPtr, "=" ) ) != NULL ) {
            *tmpPtr = '\0';
            tmpPtr++;
            j = addKeyVal( kvp, valPtr, tmpPtr );
            if ( j < 0 ) {
                return j;
            }
            *tmpPtr = '=';
        }
    }
    outKeyValPairP->inOutStruct = ( void * ) kvp;
    outKeyValPairP->type = strdup( KeyValPair_MS_T );

    return 0;
}

/**
 * \fn msiString2StrArray(msParam_t *inBufferP, msParam_t* outStrArrayP, ruleExecInfo_t *rei)
 *
 * \brief  This microservice converts a %-separated strings into strArr_t structure.
 *
 * \module core
 *
 * \since 3.0
 *
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] inBufferP - a msParam of type STR_MS_T which is key=value pairs separated by %-sign.
 * \param[out] outStrArrayP - a msParam of type StrArray_MS_T which is a structure of array of string.
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
 * \sa msiStrArray2String
**/
int msiString2StrArray( msParam_t *inBufferP, msParam_t* outStrArrayP, ruleExecInfo_t *rei ) {
    strArray_t *strArray;
    char *buf;
    int i;

    RE_TEST_MACRO( "msiString2StrArray" );

    if ( inBufferP == NULL || inBufferP->inOutStruct == NULL ||
            inBufferP->type == NULL || strcmp( inBufferP->type, STR_MS_T ) != 0 ) {
        return USER_PARAM_TYPE_ERR;
    }

    buf = strdup( ( char * )  inBufferP->inOutStruct );
    strArray = ( strArray_t * ) malloc( sizeof( strArray_t ) );
    memset(strArray, 0, sizeof(strArray_t));
    i =  parseMultiStr( buf, strArray );
    free( buf );
    if ( i < 0 ) {
        free( strArray );
        return i;
    }
    outStrArrayP->inOutStruct = ( void * ) strArray;
    outStrArrayP->type = strdup( StrArray_MS_T );

    return 0;
}


/**
 * \fn msiStrArray2String(msParam_t* inSAParam, msParam_t* outStr, ruleExecInfo_t *rei)
 *
 * \brief  Array of Strings converted to a string separated by %-signs
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] inSAParam - a msParam of type strArr_MS_T which is an array of strings.
 * \param[out] outStr - a msParam of type STR_MS_T which a string with %-separators.
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
 * \sa msiString2KeyValPair
**/
int msiStrArray2String( msParam_t* inSAParam, msParam_t* outStr, ruleExecInfo_t *rei ) {
    int i;
    strArray_t *strArr;
    char *s;
    char *val;

    RE_TEST_MACRO( "msiStrArray2String" );

    strArr = ( strArray_t * ) inSAParam->inOutStruct;
    val = strArr->value;
    s = ( char * ) malloc( strArr->len * strArr->size );
    s[0] = '\0';

    strcat( s, val );
    for ( i = 1; i < strArr->len; i++ ) {
        strcat( s, "%" );
        strcat( s, &val[i * strArr->size] );
    }
    outStr->inOutStruct = ( void * ) strdup( s );
    outStr->type = strdup( STR_MS_T );
    free( s );
    return 0;
}



/**
 * \fn msiAddKeyVal(msParam_t *inKeyValPair, msParam_t *key, msParam_t *value, ruleExecInfo_t *rei)
 *
 * \brief  Adds a new key and value to a keyValPair_t
 *
 * \module core
 *
 *
 * \note A new keyValPair_t is created if inKeyValPair is NULL.
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in,out] inKeyValPair - Optional - a KeyValPair_MS_T
 * \param[in] key - Required - A STR_MS_T containing the key
 * \param[in] value - Optional - A STR_MS_T containing the value
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
int msiAddKeyVal( msParam_t *inKeyValPair, msParam_t *key, msParam_t *value, ruleExecInfo_t *rei ) {
    char *key_str, *value_str;
    keyValPair_t *newKVP;

    /*************************************  INIT **********************************/

    /* For testing mode when used with irule --test */
    RE_TEST_MACRO( "    Calling msiAddKeyVal" )

    /* Sanity checks */
    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR, "msiAddKeyVal: input rei or rsComm is NULL." );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }


    /********************************** PARAM PARSING  *********************************/

    /* Parse key */
    if ( ( key_str = parseMspForStr( key ) ) == NULL ) {
        rodsLog( LOG_ERROR, "msiAddKeyVal: input key is NULL." );
        return USER__NULL_INPUT_ERR;
    }

    /* Parse value */
    value_str = parseMspForStr( value );

    /* Check for wrong parameter type */
    if ( inKeyValPair->type && strcmp( inKeyValPair->type, KeyValPair_MS_T ) ) {
        rodsLog( LOG_ERROR, "msiAddKeyVal: inKeyValPair is not of type KeyValPair_MS_T." );
        return USER_PARAM_TYPE_ERR;
    }

    /* Parse inKeyValPair. Create new one if empty. */
    if ( !inKeyValPair->inOutStruct ) {
        /* Set content */
        newKVP = ( keyValPair_t* )malloc( sizeof( keyValPair_t ) );
        memset( newKVP, 0, sizeof( keyValPair_t ) );
        inKeyValPair->inOutStruct = ( void* )newKVP;

        /* Set type */
        if ( !inKeyValPair->type ) {
            inKeyValPair->type = strdup( KeyValPair_MS_T );
        }
    }


    /******************************* ADD NEW PAIR AND DONE ******************************/

    rei->status = addKeyVal( ( keyValPair_t* )inKeyValPair->inOutStruct, key_str, value_str );

    /* Done */
    return rei->status;
}


