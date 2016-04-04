#include "subStructFileStat.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcSubStructFileStat( rcComm_t *conn, subFile_t *subFile, rodsStat_t **subStructFileStatOut )
 *
 * \brief Stat a subfile within a structured file object.
 *
 * \ingroup server_structuredfile
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] subFile
 * \param[out] subStructFileStatOut
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcSubStructFileStat( rcComm_t *conn, subFile_t *subFile,
                     rodsStat_t **subStructFileStatOut ) {
    int status;
    status = procApiRequest( conn, SUB_STRUCT_FILE_STAT_AN, subFile, NULL,
                             ( void ** ) subStructFileStatOut, NULL );

    return status;
}
