#include "fileLseek.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileLseek( rcComm_t *conn, fileLseekInp_t *fileLseekInp, fileLseekOut_t **fileLseekOut )
 *
 * \brief Left seek within a file.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileLseekInp
 * \param[out] fileLseekOut
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileLseek( rcComm_t *conn, fileLseekInp_t *fileLseekInp,
             fileLseekOut_t **fileLseekOut ) {
    int status;
    status = procApiRequest( conn, FILE_LSEEK_AN,  fileLseekInp, NULL,
                             ( void ** ) fileLseekOut, NULL );

    return status;
}
