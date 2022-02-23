#ifndef SUB_STRUCT_FILE_STAT_H__
#define SUB_STRUCT_FILE_STAT_H__

#include "irods/rcConnect.h"
#include "irods/objInfo.h"
#include "irods/rodsType.h"

int rcSubStructFileStat( rcComm_t *conn, subFile_t *subFile, rodsStat_t **subStructFileStatOut );

#endif
