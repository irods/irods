/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
/* subStructFileRead.h
 */

#ifndef SUB_STRUCT_FILE_READ_HPP
#define SUB_STRUCT_FILE_READ_HPP

/* This is Object File I/O type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"

typedef struct SubStructFileFdOpr {
    rodsHostAddr_t addr;
    structFileType_t type;
    int fd;
    int len;
    char resc_hier[ MAX_NAME_LEN ];
} subStructFileFdOprInp_t;

#define SubStructFileFdOpr_PI "struct RHostAddr_PI; int type; int fd; int len;"

#if defined(RODS_SERVER)
#define RS_SUB_STRUCT_FILE_READ rsSubStructFileRead
/* prototype for the server handler */
int
rsSubStructFileRead( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileReadInp,
                     bytesBuf_t *subStructFileReadOutBBuf );
int
_rsSubStructFileRead( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileReadInp,
                      bytesBuf_t *subStructFileReadOutBBuf );
int
remoteSubStructFileRead( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileReadInp,
                         bytesBuf_t *subStructFileReadOutBBuf, rodsServerHost_t *rodsServerHost );
#else
#define RS_SUB_STRUCT_FILE_READ NULL
#endif

/* prototype for the client call */
int
rcSubStructFileRead( rcComm_t *conn, subStructFileFdOprInp_t *subStructFileReadInp,
                     bytesBuf_t *subStructFileReadOutBBuf );

#endif	/* SUB_STRUCT_FILE_READ_H */
