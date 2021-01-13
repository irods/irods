#include "specificQuery.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcSpecificQuery (rcComm_t *conn, specificQueryInp_t *specificQueryInp, genQueryOut_t **genQueryOut)
*
* \brief Perform a specific (pre-defined) query.
*
* \user client
*
* \ingroup metadata
*
* \since 2.5
*
*
* \remark none
*
* \note none
*
* \usage
* Perform a specific (pre-defined) query:
* \n See the SQL-Based_Queries on irods.org
* \n and examples in iquest.c (function execAndShowSpecificQuery).
*
* \param[in] conn - A rcComm_t connection handle to the server.
* \param[in] specificQueryInp - input sql or alias (must match definition on server), and arguments
* \param[out] genQueryOut - the same returned structure as general-query.
* \return integer
* \retval 0 on success
*
* \sideeffect none
* \pre none
* \post none
* \sa none
**/

int rcSpecificQuery(rcComm_t* conn, specificQueryInp_t* specificQueryInp, genQueryOut_t** genQueryOut)
{
    return procApiRequest(conn, SPECIFIC_QUERY_AN, specificQueryInp, nullptr,
                          reinterpret_cast<void**>(genQueryOut), nullptr);
}
