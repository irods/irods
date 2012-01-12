/* This is script-generated code.  */ 
/* See endTransaction.h for a description of this API call.*/

#include "endTransaction.h"

int
rcEndTransaction (rcComm_t *conn, endTransactionInp_t *endTransactionInp)
{
    int status;
    status = procApiRequest (conn, END_TRANSACTION_AN,  endTransactionInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
