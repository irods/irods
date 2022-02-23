#ifndef RS_STRUCT_FILE_SYNC_HPP
#define RS_STRUCT_FILE_SYNC_HPP

#include "irods/rodsConnect.h"
#include "irods/structFileSync.h"

int rsStructFileSync( rsComm_t *rsComm, structFileOprInp_t *structFileOprInp );
int _rsStructFileSync( rsComm_t *rsComm, structFileOprInp_t *structFileOprInp );
int remoteStructFileSync( rsComm_t *rsComm, structFileOprInp_t *structFileOprInp, rodsServerHost_t *rodsServerHost );

#endif
