#ifndef RS_STRUCT_FILE_EXTRACT_HPP
#define RS_STRUCT_FILE_EXTRACT_HPP

#include "irods/rodsConnect.h"
#include "irods/structFileSync.h"

int rsStructFileExtract( rsComm_t *rsComm, structFileOprInp_t *structFileOprInp );
int _rsStructFileExtract( rsComm_t *rsComm, structFileOprInp_t *structFileOprInp );
int remoteStructFileExtract( rsComm_t *rsComm, structFileOprInp_t *structFileOprInp, rodsServerHost_t *rodsServerHost );

#endif
