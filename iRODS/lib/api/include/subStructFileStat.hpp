/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
/* subStructFileStat.h
 */

#ifndef SUB_STRUCT_FILE_STAT_HPP
#define SUB_STRUCT_FILE_STAT_HPP

/* This is Object File I/O type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"

#if defined(RODS_SERVER)
#define RS_SUB_STRUCT_FILE_STAT rsSubStructFileStat
/* prototype for the server handler */
int
rsSubStructFileStat( rsComm_t *rsComm, subFile_t *subFile, rodsStat_t **subStructFileStatOut );
int
_rsSubStructFileStat( rsComm_t *rsComm, subFile_t *subFile,
                      rodsStat_t **subStructFileStatOut );
int
remoteSubStructFileStat( rsComm_t *rsComm, subFile_t *subFile,
                         rodsStat_t **subStructFileStatOut, rodsServerHost_t *rodsServerHost );
#else
#define RS_SUB_STRUCT_FILE_STAT NULL
#endif

/* prototype for the client call */
int
rcSubStructFileStat( rcComm_t *conn, subFile_t *subFile,
                     rodsStat_t **subStructFileStatOut );

#endif	/* SUB_STRUCT_FILE_STAT_H */
