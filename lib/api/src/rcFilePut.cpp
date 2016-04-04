#include "filePut.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFilePut( rcComm_t *conn, fileOpenInp_t *filePutInp, bytesBuf_t *filePutInpBBuf, filePutOut_t** put_out )
 *
 * \brief Basic file put operation.
 *
 * \user client
 *
 * \ingroup server_filedriver
 *
 * \since 1.0
 *
 *
 * \remark none
 *
 * \note none
*
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] filePutInp
 * \param[in] filePutInpBBuf - buffer containing the file's contents
 * \param[out] put_out
 *
 * \return integer
 * \retval 0 on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int rcFilePut(
    rcComm_t *conn,
    fileOpenInp_t *filePutInp,
    bytesBuf_t *filePutInpBBuf,
    filePutOut_t** put_out ) {
    int status;

#if defined(osx_platform) // JMC - backport 4614
    if ( filePutInp->flags & O_TRUNC ) {
        filePutInp->flags = filePutInp->flags ^ O_TRUNC;
        filePutInp->flags = filePutInp->flags | 0x200;
    }
#endif


    status = procApiRequest( conn, FILE_PUT_AN, filePutInp, filePutInpBBuf,
                             ( void ** ) put_out, NULL );

    return status;
}
