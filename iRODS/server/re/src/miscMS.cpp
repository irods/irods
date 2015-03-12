/**
 * @file  miscMS.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "reGlobalsExtern.hpp"
#include "reFuncDefs.hpp"
#include "rsGlobalExtern.hpp"
#include "dataObjCreate.hpp"
#include "objMetaOpr.hpp"
#include "regDataObj.hpp"
/* #include "reAction.hpp" */
#include "miscServerFunct.hpp"
#include "apiHeaderAll.hpp"
#include "miscUtil.hpp"


/**
 * \fn msiRegisterData(ruleExecInfo_t *rei)
 *
 * \brief Register a new data object
 *
 * \module framework
 *
 * \since pre-2.1
 *
 * \author Arcot Rajasekar
 * \date    2008
 *
 * \note  Use this only internally as data object information has to be in rei.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence $doi
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified file metadata
 * \sideeffect none
 *
 * \return integer
 * \retval 0 upon success
 * \pre none
 * \post none
 * \sa none
 **/
int
msiRegisterData( ruleExecInfo_t *rei ) {
    int status;
    dataObjInfo_t *myDataObjInfo;


    /**** This is Just a Test Stub  ****/
    if ( reTestFlag > 0 ) {
        if ( reTestFlag == COMMAND_TEST_1 || reTestFlag == HTML_TEST_1 ) {
            print_doi( rei->doi );
        }
        else {
            rodsLog( LOG_NOTICE, "   Calling chlRegDataObj\n" );
            print_doi( rei->doi );
        }
        if ( reLoopBackFlag > 0 ) {
            return 0;
        }
    }
    /**** This is Just a Test Stub  ****/

    myDataObjInfo = L1desc[rei->l1descInx].dataObjInfo;
    status = svrRegDataObj( rei->rsComm, myDataObjInfo );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "msiRegisterData: rsRegDataObj for %s failed, status = %d",
                 myDataObjInfo->objPath, status );
        return status;
    }
    else {
        myDataObjInfo->replNum = status;
        return 0;
    }
}

int
recover_msiRegisterData( ruleExecInfo_t *rei ) {


    /**** This is Just a Test Stub  ****/
    if ( reTestFlag > 0 ) {
        if ( reTestFlag == LOG_TEST_1 ) {
            rodsLog( LOG_NOTICE, "   ROLLBACK:Calling recover_chlRegDataObj\n" );
        }
        if ( reLoopBackFlag > 0 ) {
            return 0;
        }
    }
    /**** This is Just a Test Stub  ****/

    msiRollback( rei ); /* rolling back */
    return 0;

}
int
print_bye( ruleExecInfo_t *rei ) {
    RE_TEST_MACRO( "Bye\n" );
    fprintf( stdout, "Bye\n" );
    return 0;
}

int
print_eol( ruleExecInfo_t *rei ) {
    RE_TEST_MACRO( "\n" );
    fprintf( stdout, "\n" );
    return 0;
}
int
print_hello_arg( msParam_t* xs, ruleExecInfo_t *rei ) {
    char *s;

    s = ( char * ) xs->inOutStruct;
    RE_TEST_MACRO( s );
    fprintf( stdout, "%s\n", s );
    return 0;
}

int
recover_print_bye( ruleExecInfo_t *rei ) {
    RE_TEST_MACRO( "\b\b\b   \b\b\b" );
    fprintf( stdout, "\b\b\b   \b\b\b" );
    return 0;
}

int
recover_print_eol( ruleExecInfo_t *rei ) {
    RE_TEST_MACRO( "*\b" );
    fprintf( stdout, "*\b" );
    return 0;
}

int
recover_print_hello_arg( msParam_t* xs, ruleExecInfo_t* ) {
    unsigned int i;
    char *s;

    s = ( char * ) xs->inOutStruct;
    for ( i = 0; i < strlen( s ); i++ ) {
        fprintf( stdout, "\b \b" );
    }
    return 0;
}



int
msitest1( msParam_t *A, msParam_t *B, msParam_t* C, ruleExecInfo_t *rei ) {
    /* A is IN, B is IN, C is OUT */
    msParam_t *mPA, *mPB, *mPC;

    char tmpStr[200];
    mPA = ( msParam_t * ) A;
    mPB = ( msParam_t * ) B;
    mPC = ( msParam_t * ) C;

    mPC->inOutStruct = ( void * ) strdup( "msitest1 ValC" );
    mPC->type = strdup( STR_MS_T );
    snprintf( tmpStr, 199, "msitest1: In A=%s, In B=%s, Out C=%s",
              ( char * ) mPA->inOutStruct, ( char * ) mPB->inOutStruct, ( char * ) mPC->inOutStruct );
    printf( "%s\n", tmpStr );
    RE_TEST_MACRO( tmpStr );

    return 0;
}
int
msitest2( msParam_t *A, msParam_t *B, msParam_t* C, ruleExecInfo_t *rei ) {
    /* A is IN, B is OUT, C is OUT */
    msParam_t *mPA, *mPB, *mPC;

    char tmpStr[200];
    mPA = ( msParam_t * ) A;
    mPB = ( msParam_t * ) B;
    mPC = ( msParam_t * ) C;

    mPB->inOutStruct = ( void * ) strdup( "msitest2 ValB" );
    mPC->inOutStruct = ( void * ) strdup( "msitest2 ValC" );
    mPB->type = strdup( STR_MS_T );
    mPC->type = strdup( STR_MS_T );
    snprintf( tmpStr, 199, "msitest2: In A=%s, Out B=%s, Out C=%s",
              ( char * ) mPA->inOutStruct, ( char * ) mPB->inOutStruct, ( char * ) mPC->inOutStruct );
    printf( "%s\n", tmpStr );
    RE_TEST_MACRO( tmpStr );

    return 0;
}

int
msitest3( msParam_t *A, msParam_t *B, msParam_t* C, ruleExecInfo_t *rei ) {
    /* A is IN, B is IN C is IN */
    msParam_t *mPA, *mPB, *mPC;

    char tmpStr[200];
    mPA = ( msParam_t * ) A;
    mPB = ( msParam_t * ) B;
    mPC = ( msParam_t * ) C;

    snprintf( tmpStr, 199, "msitest3: In A=%s, In B=%s, In C=%s",
              ( char * ) mPA->inOutStruct, ( char * ) mPB->inOutStruct, ( char * ) mPC->inOutStruct );
    printf( "%s\n", tmpStr );
    RE_TEST_MACRO( tmpStr );

    return 0;
}



/**
 * \fn msiApplyDCMetadataTemplate(msParam_t* inpParam, msParam_t* outParam, ruleExecInfo_t *rei)
 *
 * \brief This microservice adds 15 empty Dublin Core Metadata fields to an object or collection.
 *
 * \module framework
 *
 * \since pre-2.1
 *
 * \author  Antoine de Torcy
 * \date    2008-04-04
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam - a STR_MS_T containing the target object's path
 * \param[out] outParam - an INT_MS_T containing the status
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
msiApplyDCMetadataTemplate( msParam_t* inpParam, msParam_t* outParam, ruleExecInfo_t *rei ) {
    char *objPath;
    char objType[NAME_LEN];
    modAVUMetadataInp_t modAVUMetadataInp;
    int i, status;

    /* Dublin Core metadata elements */
    char *elements[] = {"DC.Title", "DC.Creator", "DC.Subject", "DC.Description", "DC.Publisher", "DC.Contributor", "DC.Date", "DC.Type", "DC.Format", "DC.Identifier", "DC.Source", "DC.Language", "DC.Relation", "DC.Coverage", "DC.Rights" };


    /* For testing mode when used with irule --test */
    RE_TEST_MACRO( "    Calling msiApplyDCMetadataTemplate" )


    /* microservice check */
    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR, "msiApplyDCMetadataTemplate: input rei or rsComm is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }


    /* Check for proper input */
    if ( ( objPath = parseMspForStr( inpParam ) ) == NULL ) {
        rodsLog( LOG_ERROR, "msiApplyDCMetadataTemplate: input parameter is NULL" );
        return USER__NULL_INPUT_ERR;
    }


    /* Get type of target object */
    status = getObjType( rei->rsComm, objPath, objType );


    /* Add each DC element */
    for ( i = 0; i < 15; i++ ) {
        /* set up our modAVU input */
        memset( &modAVUMetadataInp, 0, sizeof( modAVUMetadataInp_t ) );
        modAVUMetadataInp.arg0 = "add";
        modAVUMetadataInp.arg1 = objType;
        modAVUMetadataInp.arg2 = objPath;
        modAVUMetadataInp.arg3 = elements[i];   /* attribute */
        modAVUMetadataInp.arg4 = " ";           /* value, cannot be empty */
        modAVUMetadataInp.arg5 = "";            /* units, can be empty */

        /* add metadata AVU triplet */
        status = rsModAVUMetadata( rei->rsComm, &modAVUMetadataInp );
    }


    /* return operation status through outParam */
    fillIntInMsParam( outParam, status );


    return status;
}

