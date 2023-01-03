#ifndef IRODS_FILE_GET_FS_FREESPACE_H
#define IRODS_FILE_GET_FS_FREESPACE_H

#include "irods/rodsDef.h"
#include "irods/rcConnect.h"

typedef struct FileGetFsFreespaceInp
{
    rodsHostAddr_t addr;
    char fileName[MAX_NAME_LEN];
    char rescHier[MAX_NAME_LEN];
    char objPath[MAX_NAME_LEN];
    int flag;
} fileGetFsFreeSpaceInp_t;
#define fileGetFsFreeSpaceInp_PI "struct RHostAddr_PI; str fileName[MAX_NAME_LEN]; str rescHier[MAX_NAME_LEN]; str objPath[MAX_NAME_LEN]; int flag;"

typedef struct FileGetFsFreespaceOut
{
    rodsLong_t size;
} fileGetFsFreeSpaceOut_t;
#define fileGetFsFreeSpaceOut_PI "double size;"

#ifdef __cplusplus
extern "C"
#endif
int rcFileGetFsFreeSpace( rcComm_t *conn, fileGetFsFreeSpaceInp_t *fileGetFsFreeSpaceInp, fileGetFsFreeSpaceOut_t **fileGetFsFreeSpaceOut );

#endif // IRODS_FILE_GET_FS_FREESPACE_H
