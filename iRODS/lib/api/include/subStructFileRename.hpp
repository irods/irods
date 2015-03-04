/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
/* subStructFileRename.h
 */

#ifndef SUB_STRUCT_FILE_RENAME_HPP
#define SUB_STRUCT_FILE_RENAME_HPP

/* This is Object File I/O type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"

typedef struct SubStructFileRenameInp {
    subFile_t subFile;
    char newSubFilePath[MAX_NAME_LEN];
    char resc_hier[ MAX_NAME_LEN ];
} subStructFileRenameInp_t;

#define SubStructFileRenameInp_PI "struct SubFile_PI; str newSubFilePath[MAX_NAME_LEN]; str resc_hier[MAX_NAME_LEN];"
#if defined(RODS_SERVER)
#define RS_SUB_STRUCT_FILE_RENAME rsSubStructFileRename
/* prototype for the server handler */
int
rsSubStructFileRename( rsComm_t *rsComm, subStructFileRenameInp_t *subStructFileRenameInp );
int
_rsSubStructFileRename( rsComm_t *rsComm, subStructFileRenameInp_t *subStructFileRenameInp );
int
remoteSubStructFileRename( rsComm_t *rsComm, subStructFileRenameInp_t *subStructFileRenameInp,
                           rodsServerHost_t *rodsServerHost );
#else
#define RS_SUB_STRUCT_FILE_RENAME NULL
#endif

/* prototype for the client call */
int
rcSubStructFileRename( rcComm_t *conn, subStructFileRenameInp_t *subStructFileRenameInp );

#endif  /* SUB_STRUCT_FILE_RENAME_H */
