#ifndef RS_SUB_STRUCT_FILE_RENAME_HPP
#define RS_SUB_STRUCT_FILE_RENAME_HPP

#include "irods/rodsConnect.h"
#include "irods/subStructFileRename.h"

int rsSubStructFileRename( rsComm_t *rsComm, subStructFileRenameInp_t *subStructFileRenameInp );
int _rsSubStructFileRename( rsComm_t *rsComm, subStructFileRenameInp_t *subStructFileRenameInp );
int remoteSubStructFileRename( rsComm_t *rsComm, subStructFileRenameInp_t *subStructFileRenameInp, rodsServerHost_t *rodsServerHost );

#endif
