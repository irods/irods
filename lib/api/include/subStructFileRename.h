#ifndef SUB_STRUCT_FILE_RENAME_H__
#define SUB_STRUCT_FILE_RENAME_H__

#include "objInfo.h"
#include "rcConnect.h"

typedef struct SubStructFileRenameInp {
    subFile_t subFile;
    char newSubFilePath[MAX_NAME_LEN];
    char resc_hier[ MAX_NAME_LEN ];
} subStructFileRenameInp_t;

#define SubStructFileRenameInp_PI "struct SubFile_PI; str newSubFilePath[MAX_NAME_LEN]; str resc_hier[MAX_NAME_LEN];"

int rcSubStructFileRename( rcComm_t *conn, subStructFileRenameInp_t *subStructFileRenameInp );

#endif
