#include "fileStat.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileStat( rcComm_t *conn, fileStatInp_t *fileStatInp, rodsStat_t **fileStatOut )
 *
 * \brief Stat a file.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileStatInp
 * \param[out] fileStatOut - the stat output
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileStat( rcComm_t *conn, fileStatInp_t *fileStatInp,
            rodsStat_t **fileStatOut ) {
    int status;
    status = procApiRequest( conn, FILE_STAT_AN,  fileStatInp, NULL,
                             ( void ** ) fileStatOut, NULL );

    return status;
}
