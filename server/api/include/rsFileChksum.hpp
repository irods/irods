#ifndef RS_FILE_CHKSUM_HPP
#define RS_FILE_CHKSUM_HPP

#include "rodsConnect.h"
#include "rodsDef.h"
#include "fileChksum.h"

int rsFileChksum( rsComm_t *rsComm, fileChksumInp_t *fileChksumInp, char **chksumStr );
int _rsFileChksum( rsComm_t *rsComm, fileChksumInp_t *fileChksumInp, char **chksumStr );
int remoteFileChksum( rsComm_t *rsComm, fileChksumInp_t *fileChksumInp, char **chksumStr, rodsServerHost_t *rodsServerHost );
int fileChksum( rsComm_t *rsComm, char* objPath, char *fileName, char* rescHier, char* orig_chksum, char *chksumStr );

#endif
