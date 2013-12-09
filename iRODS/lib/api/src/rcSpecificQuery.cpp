/**
* @file  rcSpecificQuery.c
*
*/
/*** Copyright (c), The Regents of the University of California            ***
*** For more information please refer to files in the COPYRIGHT directory ***/

/* See specificQuery.h for a description of this API call.*/


#include "specificQuery.hpp"

/**
 * \fn rcSpecificQuery (rcComm_t *conn, specificQueryInp_t *specificQueryInp, genQueryOut_t **genQueryOut)
*
* \brief Perform a specific (pre-defined) query.
*
* \user client
*
* \category metadata operations
*
* \since 2.5
*
* \author  Wayne Schroeder
* \date    2011
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
* \bug  no known bugs
**/

int
rcSpecificQuery( rcComm_t *conn, specificQueryInp_t *specificQueryInp,
                 genQueryOut_t **genQueryOut ) {
    int status;
    status = procApiRequest( conn, SPECIFIC_QUERY_AN,  specificQueryInp, NULL,
                             ( void ** )genQueryOut, NULL );

    return ( status );
}
