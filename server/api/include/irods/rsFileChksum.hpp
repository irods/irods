#ifndef RS_FILE_CHKSUM_HPP
#define RS_FILE_CHKSUM_HPP

/// \file

#include "irods/fileChksum.h"

struct RsComm;
struct rodsServerHost;

int rsFileChksum(RsComm *rsComm, fileChksumInp_t *fileChksumInp, char **chksumStr);

int _rsFileChksum(RsComm *rsComm, fileChksumInp_t *fileChksumInp, char **chksumStr);

int remoteFileChksum(RsComm *rsComm,
                     fileChksumInp_t *fileChksumInp,
                     char **chksumStr,
                     rodsServerHost *rodsServerHost);

[[deprecated("Use file_checksum.")]]
int fileChksum(RsComm *rsComm,
               char* objPath,
               char *fileName,
               char* rescHier,
               char* orig_chksum,
               char *chksumStr);

int file_checksum(RsComm* _comm,
                  const char* _logical_path,
                  const char* _filename,
                  const char* _resource_hierarchy,
                  const char* _original_checksum,
                  rodsLong_t _data_size,
                  char* _calculated_checksum);

#endif // RS_FILE_CHKSUM_HPP

