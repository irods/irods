#ifndef SUB_STRUCT_FILE_READ_H__
#define SUB_STRUCT_FILE_READ_H__

#include "rcConnect.h"
#include "rodsDef.h"
#include "objInfo.h"

typedef struct SubStructFileFdOpr {
    rodsHostAddr_t addr;
    structFileType_t type;
    int fd;
    int len;
    char resc_hier[ MAX_NAME_LEN ];
} subStructFileFdOprInp_t;
#define SubStructFileFdOpr_PI "struct RHostAddr_PI; int type; int fd; int len;"

int rcSubStructFileRead( rcComm_t *conn, subStructFileFdOprInp_t *subStructFileReadInp, bytesBuf_t *subStructFileReadOutBBuf );

#endif
