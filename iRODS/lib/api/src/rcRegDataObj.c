/* This is script-generated code.  */ 
/* See regDataObj.h for a description of this API call.*/

#include "regDataObj.h"

int
rcRegDataObj (rcComm_t *conn, dataObjInfo_t *dataObjInfo, 
dataObjInfo_t **outDataObjInfo)
{
    int status;
    rescInfo_t *srcRescInfo;
    dataObjInfo_t *srcNext;

    /* don't sent rescInfo and next */
    srcRescInfo = dataObjInfo->rescInfo;
    srcNext = dataObjInfo->next;
    dataObjInfo->rescInfo = NULL;
    dataObjInfo->next = NULL;

    status = procApiRequest (conn, REG_DATA_OBJ_AN, dataObjInfo, NULL, 
        (void **) outDataObjInfo, NULL);

    /* restore */
    dataObjInfo->rescInfo = srcRescInfo;
    dataObjInfo->next = srcNext;

    /* cleanup fake pointers */
    if (status >= 0 && *outDataObjInfo != NULL) {
	if ((*outDataObjInfo)->rescInfo != NULL) {
	    free ((*outDataObjInfo)->rescInfo);
	    (*outDataObjInfo)->rescInfo = NULL;
	}
        if ((*outDataObjInfo)->next != NULL) {
            free ((*outDataObjInfo)->next);
            (*outDataObjInfo)->next = NULL;
        }
    }
    return (status);
}
