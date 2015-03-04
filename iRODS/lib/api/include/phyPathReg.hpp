/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* phyPathReg.h
 */

#ifndef PHY_PATH_REG_HPP
#define PHY_PATH_REG_HPP

/* This is a Object File I/O API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "dataObjInpOut.hpp"

#if defined(RODS_SERVER)
#define RS_PHY_PATH_REG rsPhyPathReg
/* prototype for the server handler */
int
rsPhyPathReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp );
int
phyPathRegNoChkPerm( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp );
int
irsPhyPathReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp );
int
remotePhyPathReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp,
                  rodsServerHost_t *rodsServerHost );
int
_rsPhyPathReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp,
               const char *_resc_name, rodsServerHost_t *rodsServerHost );
int
filePathReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp, const char *_resc_name );
int
filePathRegRepl( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp, char *filePath,
                 const char *_resc_name );
int
dirPathReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp, char *filePath,
            const char *_resc_name );
int
mountFileDir( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp, char *filePath,
              const char *rescVaultPath );
int
structFileReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp );
int
unmountFileDir( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp );
int
structFileSupport( rsComm_t *rsComm, char *collection, char *collType,
                   char* );
int
linkCollReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp );
#else
#define RS_PHY_PATH_REG NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */
int
rcPhyPathReg( rcComm_t *conn, dataObjInp_t *phyPathRegInp );

/* rcPhyPathReg - Reg a iRODS data object.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInp_t *dataObjInp - generic dataObj input. Relevant items are:
 *	objPath - the path of the data object.
 *
 * OutPut -
 *   int status - The status of the operation.
 */

#ifdef __cplusplus
}
#endif
#endif	/* PHY_PATH_REG_H */
