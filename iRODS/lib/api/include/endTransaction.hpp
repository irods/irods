/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* endTransaction.h
 */

/* This client/server call is used for ending ICAT transactions, via
 * either a commit or rollback.  */

#ifndef END_TRANSACTION_HPP
#define END_TRANSACTION_HPP

/* This is a Metadata type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "icatDefines.hpp"

typedef struct {
    char *arg0;
    char *arg1;
} endTransactionInp_t;

#define endTransactionInp_PI "str *arg0; str *arg1;"

#if defined(RODS_SERVER)
#define RS_END_TRANSACTION rsEndTransaction
/* prototype for the server handler */
int
rsEndTransaction( rsComm_t *rsComm, endTransactionInp_t *endTransactionInp );

int
_rsEndTransaction( rsComm_t *rsComm, endTransactionInp_t *endTransactionInp );
#else
#define RS_END_TRANSACTION NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */
int
rcEndTransaction( rcComm_t *conn, endTransactionInp_t *endTransactionInp );
#ifdef __cplusplus
}
#endif
#endif	/* END_TRANSACTION_H */
