/**
 * @file  extractAvuMS.cpp
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

//#include "reGlobalsExtern.hpp"
#include "rcMisc.h"
//#include "reFuncDefs.hpp"
#include "objMetaOpr.hpp"
#include "miscServerFunct.hpp"

#if defined(solaris_platform)
#include <libgen.h>
#endif


#include <algorithm> // for std::find
#include <vector>
#include <string>
#include <regex.h>

#include "irods_re_structs.hpp"
#include "rsModAVUMetadata.hpp"

extern char *__loc1;


/**
 * \fn msiReadMDTemplateIntoTagStruct(msParam_t* bufParam, msParam_t* tagParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice parses a buffer containing a template-style file
 *  and stores the tags in a tag structure.
 *
 *
 * \note  The template buffer should contain triples of the form:
 *  \<PRETAG\>re1\</PRETAG\>kw\<POSTTAG\>re2\</POSTTAG\>.
 *  re1 identifies the pre-string and re2 identifies the post-string and
 *  any value between re1 and re2 in a metadata buffer can be
 *  associated with keyword kw.
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] bufParam - a msParam of type BUF_LEN_MS_T
 * \param[out] tagParam - a return msParam of type TagStruct_MS_T
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
 * \retval USER_PARAM_TYP_ERROR when input parameter doesn't match the type
 * \retval INVALID_REGEXP if the tags are not correct
 * \retval NO_VALUES_FOUND if there are no tags identified
 * \retval from addTagStruct
 * \pre none
 * \post none
 * \sa addTagStruct
**/
int
msiReadMDTemplateIntoTagStruct( msParam_t* bufParam, msParam_t* tagParam, ruleExecInfo_t *rei ) {

    bytesBuf_t *tmplObjBuf;
    tagStruct_t *tagValues;

    char *t, *t1, *t2, *t3, *t4, *t5, *t6, *t7, *t8;
    int i, j;
    regex_t preg[4];
    regmatch_t pm[4];
    char errbuff[100];

    RE_TEST_MACRO( "Loopback on msiReadMDTemplateIntoTagStruct" );

    if ( strcmp( bufParam->type, BUF_LEN_MS_T ) != 0 ||
            bufParam->inpOutBuf == NULL ) {
        return USER_PARAM_TYPE_ERR;
    }
    tmplObjBuf = ( bytesBuf_t * ) bufParam->inpOutBuf;
    j = regcomp( &preg[0], "<PRETAG>", REG_EXTENDED );
    if ( j != 0 ) {
        regerror( j, &preg[0], errbuff, sizeof( errbuff ) );
        rodsLog( LOG_NOTICE, "msiReadMDTemplateIntoTagStruct: Error in regcomp: %s\n", errbuff );
        return INVALID_REGEXP;
    }
    j = regcomp( &preg[1], "</PRETAG>", REG_EXTENDED );
    if ( j != 0 ) {
        regerror( j, &preg[1], errbuff, sizeof( errbuff ) );
        rodsLog( LOG_NOTICE, "msiReadMDTemplateIntoTagStruct: Error in regcomp: %s\n", errbuff );
        return INVALID_REGEXP;
    }
    j = regcomp( &preg[2], "<POSTTAG>", REG_EXTENDED );
    if ( j != 0 ) {
        regerror( j, &preg[2], errbuff, sizeof( errbuff ) );
        rodsLog( LOG_NOTICE, "msiReadMDTemplateIntoTagStruct: Error in regcomp: %s\n", errbuff );
        return INVALID_REGEXP;
    }
    j = regcomp( &preg[3], "</POSTTAG>", REG_EXTENDED );
    if ( j != 0 ) {
        regerror( j, &preg[3], errbuff, sizeof( errbuff ) );
        rodsLog( LOG_NOTICE, "msiReadMDTemplateIntoTagStruct: Error in regcomp: %s\n", errbuff );
        return INVALID_REGEXP;
    }

    t = ( char* )malloc( tmplObjBuf->len + 1 );
    t[tmplObjBuf->len] = '\0';
    memcpy( t, tmplObjBuf->buf, tmplObjBuf->len );
    tagValues = ( tagStruct_t* )malloc( sizeof( tagStruct_t ) );
    memset(tagValues,0,sizeof(tagStruct_t));
    tagValues->len = 0;
    t1 = t;
    while ( regexec( &preg[0], t1, 1, &pm[0], 0 ) == 0 ) {
        t2 = t1 + pm[0].rm_eo ;                        /* t2 starts preTag */
        if ( regexec( &preg[1], t2, 1, &pm[1], 0 ) != 0 ) {
            break;
        }
        t3 = t2 + pm[1].rm_eo ;                        /* t3 starts keyValue */
        t6 = t2 + pm[1].rm_so;                            /* t6 ends preTag */
        *t6 = '\0';
        if ( regexec( &preg[2], t3, 1, &pm[2], 0 ) != 0 ) {
            break;
        }
        t5 = t3 + pm[2].rm_eo ;                        /* t5 starts postTag */
        t4 = t3 + pm[2].rm_so;                            /* t4 ends keyValue */
        *t4 = '\0';
        if ( regexec( &preg[3], t5, 1, &pm[3], 0 ) != 0 ) {
            break;
        }
        t7 = t5 + pm[3].rm_eo;                        /* t7 ends the line */
        t8 = t5 + pm[3].rm_so;                            /* t8 ends postTag */
        *t8 = '\0';
        /***    rodsLog(LOG_NOTICE,"msiReadMDTemplateIntoTagStruct:TAGS:%s::%s::%s::\n",t2, t5, t3);***/
        i = addTagStruct( tagValues, t2, t5, t3 );
        if ( i != 0 ) {
            return i;
        }
        t1 = t7;
        if ( *t1 == '\0' ) {
            break;
        }
    }

    /*
    free(preg[0]);
    free(preg[1]);
    free(preg[2]);
    free(preg[3]);
    */
    regfree( &preg[0] );
    regfree( &preg[1] );
    regfree( &preg[2] );
    regfree( &preg[3] );
    free( t );

    if ( tagValues->len == 0 ) {
        free( tagValues );
        return NO_VALUES_FOUND;
    }

    tagParam->inOutStruct = ( void * ) tagValues;
    tagParam->type = strdup( TagStruct_MS_T );

    return 0;

}

/**
 * \fn msiGetTaggedValueFromString (msParam_t *inTagParam, msParam_t *inStrParam, msParam_t *outValueParam, ruleExecInfo_t *)
 *
 * \brief   This microservice gets a tagged value from a string; given a tag-name gets the value from a file in tagged-format (pseudo-XML).
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \note This performs some regular expression matching. Given a regular expression as a tag-value 't', it identifies the
 * corresponding string in the match string with a string that matches a sub-string value: '\<t\>.*\</t\>'.
 * The service is used for processing a tagged structure.
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] inTagParam - a msParam of type STR_MS_T
 * \param[in] inStrParam - a msParam of type STR_MS_T
 * \param[out] outValueParam - a msParam of type INT_MS_T
 * \param[in,out] - The RuleExecInfo structure that is automatically
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
int msiGetTaggedValueFromString( msParam_t *inTagParam, msParam_t *inStrParam,
                                 msParam_t *outValueParam, ruleExecInfo_t* ) {

    int j;
    regex_t preg[2];
    regmatch_t pm[2];
    char errbuff[100];
    char *pstr[2];
    char *t1, *t2, *t3;
    char c;

    t1 = ( char * ) inStrParam->inOutStruct;
    pstr[0] = ( char * ) malloc( strlen( ( char* )inTagParam->inOutStruct ) + 6 );
    sprintf( pstr[0], "<%s>", ( char * ) inTagParam->inOutStruct );
    j = regcomp( &preg[0], pstr[0], REG_EXTENDED );
    if ( j != 0 ) {
        regerror( j, &preg[0], errbuff, sizeof( errbuff ) );
        rodsLog( LOG_NOTICE, "msiGetTaggedValueFromString: Error in regcomp: %s\n", errbuff );
        regfree( &preg[0] );
        free( pstr[0] );
        return INVALID_REGEXP;
    }
    pstr[1] = ( char * ) malloc( strlen( ( char* )inTagParam->inOutStruct ) + 6 );
    sprintf( pstr[1], "</%s>", ( char * ) inTagParam->inOutStruct );
    j = regcomp( &preg[1], pstr[1], REG_EXTENDED );
    if ( j != 0 ) {
        regerror( j, &preg[1], errbuff, sizeof( errbuff ) );
        rodsLog( LOG_NOTICE, "msiGetTaggedValueFromString: Error in regcomp: %s\n", errbuff );
        regfree( &preg[0] );
        regfree( &preg[1] );
        free( pstr[0] );
        free( pstr[1] );
        return INVALID_REGEXP;
    }
    /*    rodsLog (LOG_NOTICE,"TTTTT:%s",t1);*/
    if ( regexec( &preg[0], t1, 1, &pm[0], 0 ) == 0 ) {
        t2 = t1 + pm[0].rm_eo ;                     /* t2 starts value */
        if ( regexec( &preg[1], t2, 1, &pm[1], 0 ) != 0 ) {
            fillMsParam( outValueParam, NULL, STR_MS_T, 0, NULL );
        }
        else {
            t3 = t2 + pm[1].rm_so;                      /* t3 ends value */
            c = *t3;
            *t3 = '\0';
            fillMsParam( outValueParam, NULL, STR_MS_T, t2, NULL );
            *t3 = c;
        }
    }
    else {
        fillMsParam( outValueParam, NULL, STR_MS_T, 0, NULL );
    }
    regfree( &preg[0] );
    regfree( &preg[1] );
    free( pstr[0] );
    free( pstr[1] );
    return 0;
}

/**
 * \fn msiExtractTemplateMDFromBuf(msParam_t* bufParam, msParam_t* tagParam, msParam_t *metadataParam, ruleExecInfo_t *rei)
 *
 * \brief   This microservice parses a buffer containing metadata
 *  and uses the tags to identify Key-Value Pairs.
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \remark Terrell Russell - msi documentation, 2009-06-17
 *
 * \note  The template structure identifies triplets
 *  <pre-string-regexp,post-string-regexp,keyword> and the metadata buffer
 *  is searched for the pre and post regular expressions and the string
 *  between them is associated with the keyword.
 *  A.l  <key,value> pairs found are stored in keyValPair_t structure.
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] bufParam - a msParam of type BUF_MS_T
 * \param[in] tagParam - a msParam of type TagStruct_MS_T
 * \param[out] metadataParam - a msParam of type KeyValPair_MS_T
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
 * \retval USER_PARAM_TYP_ERROR when input parameter doesn't match the type
 * \retval INVALID_REGEXP if the tags are not correct
 * \retval from addKeyVal
 * \pre none
 * \post none
 * \sa addKeyVal
**/
int
msiExtractTemplateMDFromBuf( msParam_t* bufParam, msParam_t* tagParam,
                             msParam_t *metadataParam, ruleExecInfo_t *rei ) {


    bytesBuf_t *metaObjBuf;
    tagStruct_t *tagValues;
    keyValPair_t *metaDataPairs;
    /*  int l1, l2; */
    int i, j;
    /*char *preg[2]; */
    regex_t preg[2];
    regmatch_t pm[2];
    char errbuff[100];
    char *t, *t1, *t2, *t3, *t4;
    char c;

    RE_TEST_MACRO( "Loopback on msiExtractTemplateMetadata" );

    if ( strcmp( bufParam->type, BUF_LEN_MS_T ) != 0 ||
            bufParam->inpOutBuf == NULL ) {
        return USER_PARAM_TYPE_ERR;
    }
    if ( strcmp( tagParam->type, TagStruct_MS_T ) != 0 ) {
        return USER_PARAM_TYPE_ERR;
    }
    tagValues = ( tagStruct_t * ) tagParam->inOutStruct;
    metaObjBuf = ( bytesBuf_t * )  bufParam->inpOutBuf;
    t = ( char* )malloc( metaObjBuf->len + 1 );
    t[metaObjBuf->len] = '\0';
    memcpy( t, metaObjBuf->buf, metaObjBuf->len );
    metaDataPairs = ( keyValPair_t* )malloc( sizeof( keyValPair_t ) );
    memset(metaDataPairs,0,sizeof(keyValPair_t));
    t1 = t;
    for ( i = 0; i  < tagValues->len ; i++ ) {
        t1 = t;
        j = regcomp( &preg[0], tagValues->preTag[i], REG_EXTENDED );
        if ( j != 0 ) {
            regerror( j, &preg[0], errbuff, sizeof( errbuff ) );
            rodsLog( LOG_NOTICE, "msiExtractTemplateMDFromBuf: Error in regcomp: %s\n", errbuff );
            clearKeyVal( metaDataPairs );
            free( metaDataPairs );
            free( t );
            return INVALID_REGEXP;
        }
        j = regcomp( &preg[1], tagValues->postTag[i], REG_EXTENDED );
        if ( j != 0 ) {
            regerror( j, &preg[1], errbuff, sizeof( errbuff ) );
            rodsLog( LOG_NOTICE, "msiExtractTemplateMDFromBuf: Error in regcomp: %s\n", errbuff );
            clearKeyVal( metaDataPairs );
            free( metaDataPairs );
            free( t );
            return INVALID_REGEXP;
        }
        while ( regexec( &preg[0], t1, 1, &pm[0], 0 ) == 0 ) {
            t2 = t1 + pm[0].rm_eo ;                     /* t2 starts value */
            if ( regexec( &preg[1], t2, 1, &pm[1], 0 ) != 0 ) {
                break;
            }
            t4 = t2 + pm[1].rm_so;                      /* t4 ends value */
            t3 = t2 + pm[1].rm_eo;
            c = *t4;
            *t4 = '\0';
            /***      rodsLog(LOG_NOTICE,"msiExtractTemplateMDFromBuf:KVAL:%s::%s::\n",tagValues->keyWord[i], t2); ***/
            j = addKeyVal( metaDataPairs, tagValues->keyWord[i], t2 );
            *t4 = c;
            if ( j != 0 ) {
                free( t );
                return j;
            }
            t1 = t3;
            if ( *t1 == '\0' ) {
                break;
            }
        }
        regfree( &preg[0] );
        regfree( &preg[1] );

        continue;
    }

    free( t );

    metadataParam->inOutStruct = ( void * ) metaDataPairs;
    metadataParam->type = strdup( KeyValPair_MS_T );

    return 0;
}

/**
 * \fn msiAssociateKeyValuePairsToObj(msParam_t *metadataParam, msParam_t* objParam,
 *        msParam_t* typeParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice associates <key,value> p pairs
 *  from a given keyValPair_t structure with an object.
 *
 * \module framework
 *
 * \since pre-2.1
 *
 *
 * \note The object type is also needed:
 *  \li -d for data object
 *  \li -R for resource
 *  \li -G for resource group
 *  \li -C for collection
 *  \li -u for user
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] metadataParam - a msParam of type KeyValPair_MS_T
 * \param[in] objParam - a msParam of type STR_MS_T
 * \param[in] typeParam - a msParam of type STR_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified AVU pairs are associated  with an iRODS object
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \retval USER_PARAM_TYP_ERROR when input parameters don't match the type from addAVUMetadataFromKVPairs
 * \pre none
 * \post none
 * \sa addAVUMetadataFromKVPairs
**/
int
msiAssociateKeyValuePairsToObj( msParam_t *metadataParam, msParam_t* objParam,
                                msParam_t* typeParam,
                                ruleExecInfo_t *rei ) {


    char *objName;
    char *objType;
    /*  int id, i;*/
    int i;

    RE_TEST_MACRO( "Loopback on msiAssociateKeyValuePairsToObj" );

    if ( strcmp( metadataParam->type, KeyValPair_MS_T ) != 0 ) {
        return USER_PARAM_TYPE_ERR;
    }
    if ( strcmp( objParam->type, STR_MS_T ) != 0 ) {
        return USER_PARAM_TYPE_ERR;
    }
    if ( strcmp( typeParam->type, STR_MS_T ) != 0 ) {
        return USER_PARAM_TYPE_ERR;
    }
    objName = ( char * ) objParam->inOutStruct;
    objType = ( char * ) typeParam->inOutStruct;
    i = addAVUMetadataFromKVPairs( rei->rsComm,  objName, objType,
                                   ( keyValPair_t * ) metadataParam->inOutStruct );
    return i;

}

// =-=-=-=-=-=-=-
// JMC - backport 4836
/**
 * \fn msiSetKeyValuePairsToObj(msParam_t *metadataParam, msParam_t* objParam,
 *        msParam_t* typeParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice associates or modifies <key,value> p pairs
 *  from a given keyValPair_t structure with an object.
 *
 * \module framework
 *
 * \since 3.1
 *
 *
 * \note The object type is also needed:
 *  \li -d for data object
 *  \li -R for resource
 *  \li -G for resource group
 *  \li -C for collection
 *  \li -u for user
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] metadataParam - a msParam of type KeyValPair_MS_T
 * \param[in] objParam - a msParam of type STR_MS_T
 * \param[in] typeParam - a msParam of type STR_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified AVU pairs are associated with an iRODS object
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \retval USER_PARAM_TYP_ERROR when input parameters don't match the type from addAVUMetadataFromKVPairs
 * \pre none
 * \post none
 * \sa
**/
int
msiSetKeyValuePairsToObj( msParam_t *metadataParam, msParam_t* objParam,
                          msParam_t* typeParam,
                          ruleExecInfo_t *rei ) {
    char *objName;
    char *objType;
    int ret;

    RE_TEST_MACRO( "Loopback on msiSetKeyValuePairsToObj" );

    if ( strcmp( metadataParam->type, KeyValPair_MS_T ) != 0 ) {
        return USER_PARAM_TYPE_ERR;
    }
    if ( strcmp( objParam->type, STR_MS_T ) != 0 ) {
        return USER_PARAM_TYPE_ERR;
    }
    if ( strcmp( typeParam->type, STR_MS_T ) != 0 ) {
        return USER_PARAM_TYPE_ERR;
    }
    objName = ( char * ) objParam->inOutStruct;
    objType = ( char * ) typeParam->inOutStruct;
    ret = setAVUMetadataFromKVPairs( rei->rsComm,  objName, objType,
                                     ( keyValPair_t * ) metadataParam->inOutStruct );
    return ret;

}
// =-=-=-=-=-=-=-

/**
 * \fn msiGetObjType(msParam_t *objParam, msParam_t *typeParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice gets an object's type from the iCAT.
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] objParam  - a msParam of type STR_MS_T, the path of the iRODS object
 * \param[out] typeParam - a msParam of type STR_MS_T, will be set by this microservice
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
 * \retval USER_PARAM_TYP_ERROR when input parameter doesn't match the type
 * \retval  getObjType
 * \pre none
 * \post none
 * \sa none
**/
int
msiGetObjType( msParam_t *objParam, msParam_t *typeParam,
               ruleExecInfo_t *rei ) {


    char *objName;
    char objType[MAX_NAME_LEN];
    int i;

    RE_TEST_MACRO( "Loopback on msiGetObjType" );

    if ( strcmp( objParam->type, STR_MS_T ) != 0 ) {
        return USER_PARAM_TYPE_ERR;
    }
    objName = ( char * ) objParam->inOutStruct;

    i = getObjType( rei->rsComm, objName, objType );
    if ( i < 0 ) {
        return i;
    }
    typeParam->inOutStruct = ( char * ) strdup( objType );
    typeParam->type = strdup( STR_MS_T );
    return 0;
}


/**
 * \fn msiRemoveKeyValuePairsFromObj(msParam_t *metadataParam, msParam_t* objParam,
 *                              msParam_t* typeParam,
 *                              ruleExecInfo_t *rei)
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \brief This microservice removes with an object <key,value> p pairs
 *  from a given keyValPair_t structure.
 *
 *
 * \note The object type is also needed:
 *  \li -d for data object
 *  \li -R for resource
 *  \li -G for resource group
 *  \li -C for collection
 *  \li -u for user
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] metadataParam - a msParam of type KeyValPair_MS_T
 * \param[in] objParam - a msParam of type STR_MS_T
 * \param[in] typeParam - a msParam of type STR_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified AVU pairs removed
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \retval USER_PARAM_TYP_ERROR when input parameter doesn't match the type
 * \retval from removeAVUMetadataFromKVPairs
 * \pre none
 * \post none
 * \sa removeAVUMetadataFromKVPairs
**/
int
msiRemoveKeyValuePairsFromObj( msParam_t *metadataParam, msParam_t* objParam,
                               msParam_t* typeParam,
                               ruleExecInfo_t *rei ) {

    char *objName;
    char *objType;
    /*  int id, i;*/
    int i;

    RE_TEST_MACRO( "Loopback on msiRemoveKeyValuePairsFromObj" );

    if ( strcmp( metadataParam->type, KeyValPair_MS_T ) != 0 ) {
        return USER_PARAM_TYPE_ERR;
    }
    if ( strcmp( objParam->type, STR_MS_T ) != 0 ) {
        return USER_PARAM_TYPE_ERR;
    }
    if ( strcmp( typeParam->type, STR_MS_T ) != 0 ) {
        return USER_PARAM_TYPE_ERR;
    }
    objName = ( char * ) objParam->inOutStruct;
    objType = ( char * ) typeParam->inOutStruct;
    i = removeAVUMetadataFromKVPairs( rei->rsComm,  objName, objType,
                                      ( keyValPair_t * ) metadataParam->inOutStruct );
    return i;

}

/**
 * \fn msiModAVUMetadata(
 *  msParam_t* _item_type,
 *  msParam_t* _item_name,
 *  msParam_t* _avu_op,
 *  msParam_t* _attr_name,
 *  msParam_t* _attr_val,
 *  msParam_t* _attr_unit,
 *  ruleExecInfo_t *rei)
 *
 * \module core
 *
 * \since 4.2.7
 *
 * \brief This microservice adds, modifies or removes AVUs on an object
 *
 *
 * \note The object type is also needed:
 *  \li -d for data object
 *  \li -R for resource
 *  \li -G for resource group
 *  \li -C for collection
 *  \li -u for user
 *
 * \note avu_op is one of:
 *  \li "add"
 *  \li "set"
 *  \li "rm"
 *  \li "rmw"
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] item_type - a msParam of type STR_MS_T
 * \param[in] item_name - a msParam of type STR_MS_T
 * \param[in] avu_op    - a msParam of type STR_MS_T
 * \param[in] attrNameParam - a msParam of type STR_MS_T
 * \param[in] attrValueParam - a msParam of type STR_MS_T
 * \param[in] attrUnitsParam - a msParam of type STR_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified AVU triples add / set / rm / rmw (rm-with-wildcard)
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \retval USER_PARAM_TYP_ERROR when input parameter doesn't match the type
 * \retval from rsModAVUMetadata
 * \pre none
 * \post none
 * \sa rsModAVUMetadata
**/

int
msiModAVUMetadata(
    msParam_t* _item_type,
    msParam_t* _item_name,
    msParam_t* _avu_op,
    msParam_t* _attr_name,
    msParam_t* _attr_val,
    msParam_t* _attr_unit,
    ruleExecInfo_t* _rei )
{
    char *it_str = parseMspForStr( _item_type );
    if( !it_str ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    char *in_str = parseMspForStr( _item_name );
    if( !in_str ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    static const std::vector<std::string> v { "add", "set", "rm", "rmw" };
    char *op_str = parseMspForStr( _avu_op );
    if( !op_str || std::find(v.begin(), v.end(), op_str) == v.end()) {
        return SYS_INVALID_INPUT_PARAM;
    }

    char *an_str = parseMspForStr( _attr_name );
    if( !an_str ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    char *av_str = parseMspForStr( _attr_val );
    if( !av_str ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    char *au_str = parseMspForStr( _attr_unit );
    if( !au_str ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    modAVUMetadataInp_t avuOp;
    memset(&avuOp, 0, sizeof(avuOp));
    avuOp.arg0 = op_str;
    avuOp.arg1 = it_str;
    avuOp.arg2 = in_str;
    avuOp.arg3 = an_str;
    avuOp.arg4 = av_str;
    avuOp.arg5 = au_str;

    _rei->status = rsModAVUMetadata(_rei->rsComm, &avuOp);

    return _rei->status;
}
