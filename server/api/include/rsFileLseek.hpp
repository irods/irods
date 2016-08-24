#ifndef RS_FILE_LSEEK_HPP
#define RS_FILE_LSEEK_HPP

#include "fileLseek.h"

int rsFileLseek( rsComm_t *rsComm, fileLseekInp_t *fileLseekInp, fileLseekOut_t **fileLseekOut );
int _rsFileLseek( rsComm_t *rsComm, fileLseekInp_t *fileLseekInp, fileLseekOut_t **fileLseekOut );
int remoteFileLseek( rsComm_t *rsComm, fileLseekInp_t *fileLseekInp, fileLseekOut_t **fileLseekOut, rodsServerHost_t *rodsServerHost );

#endif
