#ifndef RS_FILE_STAT_HPP
#define RS_FILE_STAT_HPP

#include "rodsConnect.h"
#include "rodsDef.h"
#include "rcConnect.h"
#include "fileStat.h"

int rsFileStat( rsComm_t *rsComm, fileStatInp_t *fileStatInp, rodsStat_t **fileStatOut );
int _rsFileStat( rsComm_t *rsComm, fileStatInp_t *fileStatInp, rodsStat_t **fileStatOut );
int rsFileStatByHost( rsComm_t *rsComm, fileStatInp_t *fileStatInp, rodsStat_t **fileStatOut, rodsServerHost_t *rodsServerHost );
int remoteFileStat( rsComm_t *rsComm, fileStatInp_t *fileStatInp, rodsStat_t **fileStatOut, rodsServerHost_t *rodsServerHost );

#endif
