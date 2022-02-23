#ifndef RS_FILE_STAT_HPP
#define RS_FILE_STAT_HPP

#include "irods/rodsConnect.h"
#include "irods/rodsDef.h"
#include "irods/rcConnect.h"
#include "irods/fileStat.h"

int rsFileStat( rsComm_t *rsComm, fileStatInp_t *fileStatInp, rodsStat_t **fileStatOut );
int _rsFileStat( rsComm_t *rsComm, fileStatInp_t *fileStatInp, rodsStat_t **fileStatOut );
int rsFileStatByHost( rsComm_t *rsComm, fileStatInp_t *fileStatInp, rodsStat_t **fileStatOut, rodsServerHost_t *rodsServerHost );
int remoteFileStat( rsComm_t *rsComm, fileStatInp_t *fileStatInp, rodsStat_t **fileStatOut, rodsServerHost_t *rodsServerHost );

#endif
