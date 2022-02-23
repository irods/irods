#ifndef FILE_CHKSUM_H__
#define FILE_CHKSUM_H__

/// \file

#include "irods/rodsDef.h"
#include "irods/objInfo.h"
#include "irods/rodsType.h"

struct RcComm;

typedef struct FileChksumInp {
    rodsHostAddr_t addr;
    char fileName[MAX_NAME_LEN];
    char rescHier[MAX_NAME_LEN];
    char objPath[MAX_NAME_LEN];
    int flag;                   // Not used for now.
    char in_pdmo[MAX_NAME_LEN]; // Flag indicating if we are being executed from a pdmo.
    char orig_chksum[NAME_LEN]; // Original incoming checksum.
    rodsLong_t dataSize;
    struct KeyValPair condInput;
} fileChksumInp_t;

#define fileChksumInp_PI "struct RHostAddr_PI; str fileName[MAX_NAME_LEN]; str rescHier[MAX_NAME_LEN]; str objPath[MAX_NAME_LEN]; int flags; str in_pdmo[MAX_NAME_LEN]; str orig_chksum[NAME_LEN]; double dataSize; struct KeyValPair_PI;"
#define fileChksumOut_PI "str chksumStr[NAME_LEN];"

#ifdef __cplusplus
extern "C" {
#endif

/// Calculate a checksum on a file.
/// 
/// \ingroup server_filedriver
/// 
/// \param[in]  conn          A RcComm connection handle to the server.
/// \param[in]  fileChksumInp The input structure.
/// \param[out] chksumStr     The calculated checksum result.
/// 
/// \return An integer.
/// \retval 0        On success
/// \retval Non-zero Otherwise.
/// 
/// \since 3.3.0
int rcFileChksum(struct RcComm* conn, struct FileChksumInp* fileChksumInp, char** chksumStr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FILE_CHKSUM_H__

