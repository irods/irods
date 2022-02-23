#ifndef RS_SUB_STRUCT_FILE_STAT_HPP
#define RS_SUB_STRUCT_FILE_STAT_HPP

#include "irods/rodsConnect.h"
#include "irods/objInfo.h"
#include "irods/rodsType.h"

int rsSubStructFileStat( rsComm_t *rsComm, subFile_t *subFile, rodsStat_t **subStructFileStatOut );
int _rsSubStructFileStat( rsComm_t *rsComm, subFile_t *subFile, rodsStat_t **subStructFileStatOut );
int remoteSubStructFileStat( rsComm_t *rsComm, subFile_t *subFile, rodsStat_t **subStructFileStatOut, rodsServerHost_t *rodsServerHost );

#endif
