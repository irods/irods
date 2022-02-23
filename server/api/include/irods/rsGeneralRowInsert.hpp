#ifndef RS_GENERAL_ROW_INSERT_HPP
#define RS_GENERAL_ROW_INSERT_HPP

#include "irods/rcConnect.h"
#include "irods/generalRowInsert.h"

int rsGeneralRowInsert( rsComm_t *rsComm, generalRowInsertInp_t *generalRowInsertInp );
int _rsGeneralRowInsert( rsComm_t *rsComm, generalRowInsertInp_t *generalRowInsertInp );

#endif
