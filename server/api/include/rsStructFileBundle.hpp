#ifndef RS_STRUCT_FILE_BUNDLE_HPP
#define RS_STRUCT_FILE_BUNDLE_HPP

#include "rodsConnect.h"
#include "structFileExtAndReg.h"

int rsStructFileBundle( rsComm_t *rsComm, structFileExtAndRegInp_t *structFileBundleInp );
int _rsStructFileBundle( rsComm_t *rsComm, structFileExtAndRegInp_t *structFileBundleInp );
int remoteStructFileBundle( rsComm_t *rsComm, structFileExtAndRegInp_t *structFileBundleInp, rodsServerHost_t *rodsServerHost );

#endif
