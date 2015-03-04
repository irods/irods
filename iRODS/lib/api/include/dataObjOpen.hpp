/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* dataObjOpen.h
 */

#ifndef DATA_OBJ_OPEN_HPP
#define DATA_OBJ_OPEN_HPP

/* This is a high level type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "dataObjInpOut.hpp"
#include "fileOpen.hpp"

/* definition for phyOpenFlag */
#define DO_PHYOPEN	0
#define DO_NOT_PHYOPEN	1
#define PHYOPEN_BY_SIZE	2

#if defined(RODS_SERVER)
#define RS_DATA_OBJ_OPEN rsDataObjOpen
/* prototype for the server handler */
int
rsDataObjOpen( rsComm_t *rsComm, dataObjInp_t *dataObjInp );
int
_rsDataObjOpenWithObjInfo( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                           int phyOpenFlag, dataObjInfo_t *dataObjInfo ); // JMC - backport 4537
int
_rsDataObjOpen( rsComm_t *rsComm, dataObjInp_t *dataObjInp );
int
dataOpen( rsComm_t *rsComm, int l1descInx );
int
l3Open( rsComm_t *rsComm, int l1descInx );
int
_l3Open( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo, int mode, int flags );
int
l3OpenByHost( rsComm_t *rsComm, int l3descInx, int flags );
int
applyPreprocRuleForOpen( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                         dataObjInfo_t **dataObjInfoHead );
int
createEmptyRepl( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                 dataObjInfo_t **dataObjInfoHead );
// =-=-=-=-=-=-=-
// JMC - backport 4590
int
procDataObjOpenForWrite( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                         dataObjInfo_t **dataObjInfoHead,
                         dataObjInfo_t **compDataObjInfo );

// JMC :: unused -- irods::error selectObjInfo( dataObjInfo_t * _dataObjInfoHead, keyValPair_t* _condInput, dataObjInfo_t** _rtn_dataObjInfo );

// =-=-=-=-=-=-=-
#else
#define RS_DATA_OBJ_OPEN NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */
/* rcDataObjOpen - Open a iRODS data object.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInp_t *dataObjInp - generic dataObj input. Relevant items are:
 *	objPath - the path of the data object.
 *	dataType - the data type of the object (optional).
 *	openFlag - O_WRONLY, O_RDONLY , etc. The O_CREAT will create the
 *          file if it does not exist.
 *	condInput - condition input (optional).
 *          DATA_TYPE_KW - "value" = the data type of the object
 *          FORCE_FLAG_KW - overwrite an existing data object
 *          REG_CHKSUM_KW - compute the checksum value
 *          VERIFY_CHKSUM_KW - compute and verify the checksum on the data.
 *          FILE_PATH_KW - "value" = the physical path of the
 *              destination file. Vaild only if O_CREAT is on.
 *          REPL_NUM_KW  - "value" = The replica number of the copy to
 *              open.
 *          DEST_RESC_NAME_KW - "value" = The destination Resource. Vaild
 *	        only if O_CREAT is on.
 *
 * OutPut -
 *   int l1descInx - an integer descriptor.
 */

int
rcDataObjOpen( rcComm_t *conn, dataObjInp_t *dataObjInp );

#ifdef __cplusplus
}
#endif
#endif	/* DATA_OBJ_OPEN_H */
