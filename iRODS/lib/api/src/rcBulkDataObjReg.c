/* This is script-generated code.  */ 
/* See bulkDataObjReg.h for a description of this API call.*/

#include "bulkDataObjReg.h"

int
rcBulkDataObjReg (rcComm_t *conn, genQueryOut_t *bulkDataObjRegInp,
genQueryOut_t **bulkDataObjRegOut)
{
    int status;
    status = procApiRequest (conn, BULK_DATA_OBJ_REG_AN, bulkDataObjRegInp, 
      NULL, (void **) bulkDataObjRegOut, NULL);

    return (status);
}
