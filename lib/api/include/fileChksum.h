#ifndef FILE_CHKSUM_H__
#define FILE_CHKSUM_H__

#include "rcConnect.h"
#include "rodsDef.h"

typedef struct FileChksumInp {
    rodsHostAddr_t addr;
    char fileName[MAX_NAME_LEN];
    char rescHier[MAX_NAME_LEN];
    char objPath[MAX_NAME_LEN];
    int flag;   // not used for now
    char in_pdmo[MAX_NAME_LEN]; // Flag indicating if we are being executed from a pdmo
    char orig_chksum[NAME_LEN]; // original incoming checksum
} fileChksumInp_t;
#define fileChksumInp_PI "struct RHostAddr_PI; str fileName[MAX_NAME_LEN]; str rescHier[MAX_NAME_LEN]; str objPath[MAX_NAME_LEN]; int flags; str in_pdmo[MAX_NAME_LEN]; str orig_chksum[NAME_LEN];"

#define fileChksumOut_PI "str chksumStr[NAME_LEN];"

#ifdef __cplusplus
extern "C"
#endif
int rcFileChksum( rcComm_t *conn, fileChksumInp_t *fileChksumInp, char **chksumStr );

#endif
