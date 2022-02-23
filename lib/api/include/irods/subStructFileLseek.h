#ifndef SUB_STRUCT_FILE_LSEEK_H__
#define SUB_STRUCT_FILE_LSEEK_H__

#include "irods/rcConnect.h"
#include "irods/objInfo.h"
#include "irods/rodsDef.h"
#include "irods/rodsType.h"
#include "irods/fileLseek.h"

typedef struct SubStructFileLseekInp {
    rodsHostAddr_t addr;
    structFileType_t type;
    int fd;
    rodsLong_t offset;
    int whence;
    char resc_hier[ MAX_NAME_LEN ];
} subStructFileLseekInp_t;
#define SubStructFileLseekInp_PI "struct RHostAddr_PI; int type; int fd; double offset; int whence;"

int rcSubStructFileLseek( rcComm_t *conn, subStructFileLseekInp_t *subStructFileLseekInp, fileLseekOut_t **subStructFileLseekOut );

#endif
