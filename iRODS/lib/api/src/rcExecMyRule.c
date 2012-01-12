/* This is script-generated code.  */ 
/* See execMyRule.h for a description of this API call.*/

#include "execMyRule.h"
#include "oprComplete.h"
#include "dataObjPut.h"
#include "dataObjGet.h"

int
rcExecMyRule (rcComm_t *conn, execMyRuleInp_t *execMyRuleInp, 
msParamArray_t **outParamArray)
{
    int status;
    char myDir[MAX_NAME_LEN], myFile[MAX_NAME_LEN];

    status = procApiRequest (conn, EXEC_MY_RULE_AN, execMyRuleInp, NULL, 
        (void **)outParamArray, NULL);
 
    while (status == SYS_SVR_TO_CLI_MSI_REQUEST) {
	/* it is a server request */
	char *locFilePath;
        msParam_t *myMsParam;
        dataObjInp_t *dataObjInp = NULL;


	if ((myMsParam = getMsParamByLabel (*outParamArray, CL_PUT_ACTION))
	  != NULL) { 

	    dataObjInp = (dataObjInp_t *) myMsParam->inOutStruct;
	    if ((locFilePath = getValByKey (&dataObjInp->condInput, 
	      LOCAL_PATH_KW)) == NULL) {
                if ((status = splitPathByKey (dataObjInp->objPath,
                  myDir, myFile, '/')) < 0) {
                    rodsLogError (LOG_ERROR, status,
                      "rcExecMyRule: splitPathByKey for %s error",
                      dataObjInp->objPath);
                    rcOprComplete (conn, USER_FILE_DOES_NOT_EXIST);
                } else {
                    locFilePath = (char *) myFile;
                }
	    }
	    status = rcDataObjPut (conn, dataObjInp, locFilePath);
	    rcOprComplete (conn, status);
	} else if ((myMsParam = getMsParamByLabel (*outParamArray, 
	  CL_GET_ACTION)) != NULL) {
            dataObjInp = (dataObjInp_t *) myMsParam->inOutStruct;
            if ((locFilePath = getValByKey (&dataObjInp->condInput,
              LOCAL_PATH_KW)) == NULL) {
                if ((status = splitPathByKey (dataObjInp->objPath, 
                  myDir, myFile, '/')) < 0) {
        	    rodsLogError (LOG_ERROR, status,
                      "rcExecMyRule: splitPathByKey for %s error",
                      dataObjInp->objPath);
                    rcOprComplete (conn, USER_FILE_DOES_NOT_EXIST);
		} else {
		    locFilePath = (char *) myFile;
		}
            }
            status = rcDataObjGet (conn, dataObjInp, locFilePath);
            rcOprComplete (conn, status);
	} else {
	    rcOprComplete (conn, SYS_SVR_TO_CLI_MSI_NO_EXIST);
	}
	/* free outParamArray */
	if (dataObjInp != NULL) {
	    clearKeyVal (&dataObjInp->condInput); 
	}
	clearMsParamArray (*outParamArray, 1);
	free (*outParamArray);
	*outParamArray = NULL;

	/* read the reply from the eariler call */

        status = branchReadAndProcApiReply (conn, EXEC_MY_RULE_AN, 
        (void **)outParamArray, NULL);
        if (status < 0) {
            rodsLogError (LOG_DEBUG, status,
              "rcExecMyRule: readAndProcApiReply failed. status = %d", 
	      status);
        }
    }

	 
    return (status);
}

