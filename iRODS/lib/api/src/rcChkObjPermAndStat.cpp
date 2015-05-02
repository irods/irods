#include "chkObjPermAndStat.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcChkObjPermAndStat( rcComm_t *conn, chkObjPermAndStat_t *chkObjPermAndStatInp )
 *
 * \brief Check data object permissions and stat the file.
 *
 * \ingroup server_icat
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] chkObjPermAndStatInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcChkObjPermAndStat( rcComm_t *conn, chkObjPermAndStat_t *chkObjPermAndStatInp ) {
    int status;
    status = procApiRequest( conn, CHK_OBJ_PERM_AND_STAT_AN,
                             chkObjPermAndStatInp, NULL, ( void ** ) NULL, NULL );

    return status;
}
