#include "fileChksum.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileChksum( rcComm_t *conn, fileChksumInp_t *fileChksumInp, char **chksumStr )
 *
 * \brief Calculate a checksum on a file.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileChksumInp
 * \param[out] chksumStr - the checksum
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileChksum( rcComm_t *conn, fileChksumInp_t *fileChksumInp,
              char **chksumStr ) {
    int status;
    status = procApiRequest( conn, FILE_CHKSUM_AN,  fileChksumInp, NULL,
                             ( void ** ) chksumStr, NULL );

    return status;
}
