#include "fileSyncToArch.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileSyncToArch( rcComm_t *conn, fileStageSyncInp_t *fileSyncToArchInp, fileSyncOut_t** _fn )
 *
 * \brief Syncs a file from cache back to archive.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileSyncToArchInp
 * \param[out] _fn - the filename
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileSyncToArch( rcComm_t *conn, fileStageSyncInp_t *fileSyncToArchInp, fileSyncOut_t** _fn ) {
    int status;
    status = procApiRequest( conn, FILE_SYNC_TO_ARCH_AN,
                             fileSyncToArchInp, NULL, ( void** )_fn, NULL );

    return status;
}
