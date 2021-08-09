#include "collCreate.h"
#include "procApiRequest.h"
#include "apiNumber.h"

#include <cstring>

/**
 * \fn rcCollCreate (rcComm_t *conn, collInp_t *collCreateInp)
 *
 * \brief Create a collection. This is equivalent to mkdir of UNIX.
 *
 * \user client
 *
 * \ingroup collection_object
 *
 * \since 1.0
 *
 *
 * \remark none
 *
 * \note none
 *
 * \usage
 * Create a collection /myZone/home/john/coll1/coll2 and make parent collections as
 * needed:
 * \n int status;
 * \n collInp_t collCreateInp;
 * \n memset(&collCreateInp, 0, sizeof(collCreateInp));
 * \n rstrcpy (collCreateInp.collName, "/myZone/home/john/coll1/coll2", MAX_NAME_LEN);
 * \n addKeyVal (&collCreateInp.condInput, RECURSIVE_OPR__KW, "");
 * \n status = rcCollCreate (conn, &collCreateInp);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] collCreateInp - Elements of collInp_t used :
 *    \li char \b collName[MAX_NAME_LEN] - full path of the collection.
 *    \li keyValPair_t \b condInput - keyword/value pair input. Valid keywords:
 *    \n RECURSIVE_OPR__KW - make parent collections as needed. This keyWd has no value.
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/

int rcCollCreate(rcComm_t* conn, collInp_t* collCreateInp)
{
    return procApiRequest(conn, COLL_CREATE_AN,  collCreateInp, NULL, (void**) NULL, NULL );
}
