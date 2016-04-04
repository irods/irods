/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "getMiscSvrInfo.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcGetMiscSvrInfo( rcComm_t *conn, miscSvrInfo_t **outSvrInfo )
 *
 * \brief Get miscellaneous server info.
 *
 * \user client
 *
 * \ingroup miscellaneous
 *
 * \since 1.0
 *
 *
 * \remark Get the connection stat of iRODS agents running in the
 * iRODS federation. By default, the stat of the iRODS agents on the iCAT
 * enabled server (IES) is listed. Other servers can be specified using
 * the "addr" field of procStatInp or using the RESC_NAME_KW keyword.
 *
 * \note none
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[out] outSvrInfo - A struct that contains the requested info.
 *
 * \return integer
 * \retval 0 on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcGetMiscSvrInfo( rcComm_t *conn, miscSvrInfo_t **outSvrInfo ) {
    int status;

    *outSvrInfo = NULL;

    status = procApiRequest( conn, GET_MISC_SVR_INFO_AN, NULL, NULL,
                             ( void ** ) outSvrInfo, NULL );

    return status;
}
