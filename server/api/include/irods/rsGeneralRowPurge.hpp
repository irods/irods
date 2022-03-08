#ifndef RS_GENERAL_ROW_PURGE_HPP
#define RS_GENERAL_ROW_PURGE_HPP

#include "irods/rcConnect.h"
#include "irods/generalRowPurge.h"

int rsGeneralRowPurge( rsComm_t *rsComm, generalRowPurgeInp_t *generalRowPurgeInp );
int _rsGeneralRowPurge( rsComm_t *rsComm, generalRowPurgeInp_t *generalRowPurgeInp );

#endif
