#ifndef FILE_OPENDIR_H__
#define FILE_OPENDIR_H__

#include "rodsDef.h"
#include "rcConnect.h"

typedef struct {
    char resc_name_[MAX_NAME_LEN];
    char resc_hier_[MAX_NAME_LEN];
    char objPath[MAX_NAME_LEN];
    rodsHostAddr_t addr;
    char dirName[MAX_NAME_LEN];
} fileOpendirInp_t;

#define fileOpendirInp_PI "str resc_name_[MAX_NAME_LEN]; str resc_hier_[MAX_NAME_LEN]; str objPath[MAX_NAME_LEN]; struct RHostAddr_PI; str dirName[MAX_NAME_LEN];"


#ifdef __cplusplus
extern "C"
#endif
int rcFileOpendir( rcComm_t *conn, fileOpendirInp_t *fileOpendirInp );

#endif
