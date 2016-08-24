#ifndef RS_END_TRANSACTION_HPP
#define RS_END_TRANSACTION_HPP

#include "rcConnect.h"
#include "endTransaction.h"

int rsEndTransaction( rsComm_t *rsComm, endTransactionInp_t *endTransactionInp );
int _rsEndTransaction( rsComm_t *rsComm, endTransactionInp_t *endTransactionInp );

#endif
