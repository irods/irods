#ifndef STRUCT_FILE_EXTRACT_H__
#define STRUCT_FILE_EXTRACT_H__

#include "irods/rcConnect.h"
#include "irods/structFileSync.h"

int rcStructFileExtract( rcComm_t *conn, structFileOprInp_t *structFileOprInp );
int procCacheDir( rsComm_t *rsComm, char *cacheDir, char *resource, int oprType, char* hier );
#endif
