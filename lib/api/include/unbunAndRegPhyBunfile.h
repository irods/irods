#ifndef UNBUN_AND_REG_PHY_BUNFILE_HPP
#define UNBUN_AND_REG_PHY_BUNFILE_HPP

#include "rcConnect.h"
#include "dataObjInpOut.h"

#define UNLINK_FILE_AGE                7200  // delete files younger than this

/* prototype for the client call */
/* rcUnbunAndRegPhyBunfile - Unbundle a physical bundle file specified by
 * FILE_PATH_KW and register each subfile as replica. This call cannot be
 * called by normal users directly.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInp_t *dataObjInp - generic dataObj input. Relevant items are:
 *      objPath - the path of the data object.
 *      condInput - conditional Input
 *          FILE_PATH_KW - the phyical path of the bundled file.
 *          DEST_RESC_NAME_KW - "value" = The destination Resource.
 *   return value - The status of the operation.
 */
#ifdef __cplusplus
extern "C"
#endif
int rcUnbunAndRegPhyBunfile( rcComm_t *conn, dataObjInp_t *dataObjInp );

#endif
