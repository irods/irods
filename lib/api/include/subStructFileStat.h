#ifndef SUB_STRUCT_FILE_STAT_H__
#define SUB_STRUCT_FILE_STAT_H__

#include "rcConnect.h"
#include "objInfo.h"
#include "rodsType.h"

int rcSubStructFileStat( rcComm_t *conn, subFile_t *subFile, rodsStat_t **subStructFileStatOut );

#endif
