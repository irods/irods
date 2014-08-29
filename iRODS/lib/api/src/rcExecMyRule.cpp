/* This is script-generated code.  */
/* See execMyRule.h for a description of this API call.*/

#include "execMyRule.hpp"
#include "oprComplete.hpp"
#include "dataObjPut.hpp"
#include "dataObjGet.hpp"

int
rcExecMyRule( rcComm_t *conn, execMyRuleInp_t *execMyRuleInp,
              msParamArray_t **outParamArray ) {

    int status = procApiRequest( conn, EXEC_MY_RULE_AN, execMyRuleInp, NULL,
                             ( void ** )outParamArray, NULL );

    while ( status == SYS_SVR_TO_CLI_MSI_REQUEST ) {
        /* it is a server request */
        msParam_t *myParam = NULL, *putParam = NULL;

        if ( ( myParam = putParam = getMsParamByLabel( *outParamArray, CL_PUT_ACTION ) ) ||
                ( myParam = getMsParamByLabel( *outParamArray, CL_GET_ACTION ) ) ) {
            //putParam is non-null if it's a put, null if it's a get
            dataObjInp_t * dataObjInp = ( dataObjInp_t * ) myParam->inOutStruct;
            char * locFilePath;
            char myDir[MAX_NAME_LEN], myFile[MAX_NAME_LEN];
            // locFilePath should be the return of getValByKey if it exists,
            // otherwise use the filename from splitPathByKey
            if ( ( locFilePath = getValByKey( &dataObjInp->condInput, LOCAL_PATH_KW ) ) ||
                    ( ( status = splitPathByKey( dataObjInp->objPath, myDir, MAX_NAME_LEN, myFile, MAX_NAME_LEN, '/' ) ) >= 0 ) &&
                    ( locFilePath = ( char * ) myFile ) ) {
                status = putParam ?
                    rcDataObjPut( conn, dataObjInp, locFilePath ) :
                    rcDataObjGet( conn, dataObjInp, locFilePath );
                rcOprComplete( conn, status );
            }
            else {
                rodsLogError( LOG_ERROR, status,
                                "rcExecMyRule: splitPathByKey for %s error",
                                dataObjInp->objPath );
                rcOprComplete( conn, USER_FILE_DOES_NOT_EXIST );
            }
            clearKeyVal( &dataObjInp->condInput );
        }
        else {
            rcOprComplete( conn, SYS_SVR_TO_CLI_MSI_NO_EXIST );
        }
        /* free outParamArray */
        clearMsParamArray( *outParamArray, 1 );
        free( *outParamArray );
        *outParamArray = NULL;

        /* read the reply from the eariler call */

        status = branchReadAndProcApiReply( conn, EXEC_MY_RULE_AN,
                                            ( void ** )outParamArray, NULL );
        if ( status < 0 ) {
            rodsLogError( LOG_DEBUG, status,
                          "rcExecMyRule: readAndProcApiReply failed. status = %d",
                          status );
        }
    }

    return status;
}

