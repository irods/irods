#ifndef RS_SUB_STRUCT_FILE_LSEEK_HPP
#define RS_SUB_STRUCT_FILE_LSEEK_HPP

#include "rodsConnect.h"
#include "fileLseek.h"
#include "subStructFileLseek.h"


int rsSubStructFileLseek( rsComm_t *rsComm, subStructFileLseekInp_t *subStructFileLseekInp, fileLseekOut_t **subStructFileLseekOut );
int _rsSubStructFileLseek( rsComm_t *rsComm, subStructFileLseekInp_t *subStructFileLseekInp, fileLseekOut_t **subStructFileLseekOut );
int remoteSubStructFileLseek( rsComm_t *rsComm, subStructFileLseekInp_t *subStructFileLseekInp, fileLseekOut_t **subStructFileLseekOut, rodsServerHost_t *rodsServerHost );

#endif
