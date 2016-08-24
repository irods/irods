#ifndef RS_SUB_STRUCT_FILE_CLOSE_HPP
#define RS_SUB_STRUCT_FILE_CLOSE_HPP

#include "rodsConnect.h"
#include "subStructFileRead.h"

int rsSubStructFileClose( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileCloseInp );
int _rsSubStructFileClose( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileCloseInp );
int remoteSubStructFileClose( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileCloseInp, rodsServerHost_t *rodsServerHost );

#endif
