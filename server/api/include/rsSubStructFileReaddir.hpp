#ifndef RS_SUB_STRUCT_FILE_READDIR_HPP
#define RS_SUB_STRUCT_FILE_READDIR_HPP

#include "rodsConnect.h"
#include "rodsType.h"
#include "rcConnect.h"
#include "subStructFileRead.h"

int rsSubStructFileReaddir( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileReaddirInp, rodsDirent_t **rodsDirent );
int _rsSubStructFileReaddir( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileReaddirInp, rodsDirent_t **rodsDirent );
int remoteSubStructFileReaddir( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileReaddirInp, rodsDirent_t **rodsDirent, rodsServerHost_t *rodsServerHost );

#endif
