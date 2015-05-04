#include "fileStageToCache.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileStageToCache( rcComm_t *conn, fileStageSyncInp_t *fileStageToCacheInp )
 *
 * \brief Stage a file from the archive to the cache.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileStageToCacheInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileStageToCache( rcComm_t *conn, fileStageSyncInp_t *fileStageToCacheInp ) {
    int status;
    status = procApiRequest( conn, FILE_STAGE_TO_CACHE_AN,
                             fileStageToCacheInp, NULL, ( void ** ) NULL, NULL );

    return status;
}
