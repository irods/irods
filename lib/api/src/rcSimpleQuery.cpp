/**
* @file  rcSimpleQuery.cpp
*
*/
/*** Copyright (c), The Regents of the University of California            ***
*** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */
/* See simpleQuery.h for a description of this API call.*/

#include "irods/simpleQuery.h"
#include "irods/procApiRequest.h"
#include "irods/apiNumber.h"
/**
 * \fn rcSimpleQuery (rcComm_t *conn, simpleQueryInp_t *simpleQueryInp, simpleQueryOut_t **simpleQueryOut)
 *
 * \brief Perform a simple (pre-defined) query, allowed for Admin only, used in iadmin
 *
 * \user client
 *
 * \ingroup metadata
 *
 * \since 1.0
 *
 * \remark none
 *
 * \note none
 *
 * \usage
 * Perform a simple (pre-defined) query:
 * \n See the SQL-Based_Queries on irods.org
 * \n and examples in iquest.c (function execAndShowSimpleQuery).
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] simpleQueryInp - input sql or alias (must match definition on server), and arguments
 * \param[out] simpleQueryOut - the same returned structure as general-query.
 * \return integer
 * \retval 0 on success
 *
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 *
 * \deprecated Deprecated in 4.3.1. Use rcGenQuery or rcSpecificQuery instead.
 **/

int
rcSimpleQuery( rcComm_t *conn, simpleQueryInp_t *simpleQueryInp,
               simpleQueryOut_t **simpleQueryOut ) {
    int status;
    status = procApiRequest( conn, SIMPLE_QUERY_AN,  simpleQueryInp, NULL,
                             ( void ** ) simpleQueryOut, NULL );

    return status;
}
