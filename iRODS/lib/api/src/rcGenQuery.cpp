/**
 * @file  rcGenQuery.c
 *
 */
/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See genQuery.h for a description of this API call.*/

#include "genQuery.h"

/* this is a debug routine; it just prints the genQueryInp
   structure */
int
printGenQI( genQueryInp_t *genQueryInp) {
   int i, len;
   int *ip1, *ip2;
   char *cp;
   char **cpp;

   printf ("maxRows=%d\n", genQueryInp->maxRows);

   len = genQueryInp->selectInp.len;
   printf ("sel len=%d\n",len);
   ip1 = genQueryInp->selectInp.inx;
   ip2 = genQueryInp->selectInp.value;
   for (i=0;i<len;i++) {
      printf("sel inx [%d]=%d\n",i, *ip1);
      printf("sel val [%d]=%d\n",i, *ip2);
      ip1++;
      ip2++;
   }

   len = genQueryInp->sqlCondInp.len;
   printf ("sqlCond len=%d\n",len);
   ip1 = genQueryInp->sqlCondInp.inx;
   cpp = genQueryInp->sqlCondInp.value;
   cp = *cpp;
   for (i=0;i<len;i++) {
      printf("sel inx [%d]=%d\n",i, *ip1);
      printf("sel val [%d]=:%s:\n",i, cp);
      ip1++;
      cpp++;
      cp = *cpp;
   }
   return (0);
}

/**
 * \fn rcGenQuery (rcComm_t *conn, genQueryInp_t *genQueryInp, genQueryOut_t **genQueryOut)
 *
 * \brief Perform a general-query.
 *
 * \user client and server (internal queries to the ICAT as part of server ops)
 *
 * \category metadata operations
 *
 * \since .5
 *
 * \author  Wayne Schroeder
 * \date    2006
 *
 * \remark
 * Perform a general-query:
 * \n This is used extensively from within server code and from clients.
 * \n Provides a simplified interface to query the iRODS database (ICAT).
 * \n Although the inputs and controls are a bit complicated, it allows
 * \n SQL-like queries but without the caller needing to know the structure
 * \n of the database (schema).  SQL is generated for each call on the 
 * \n server-side (in the ICAT code).
 *
 * \note none
 *
 * \usage
 *
 * \param[in] conn - A rcComm_t connection handle to the server
 * \param[in] genQueryInp - input general-query structure
 * \param[out] genQueryOut - output general-query structure
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
rcGenQuery (rcComm_t *conn, genQueryInp_t *genQueryInp, 
genQueryOut_t **genQueryOut)
{
    int status;
    /*    printGenQI(genQueryInp); */
    status = procApiRequest (conn, GEN_QUERY_AN,  genQueryInp, NULL, 
        (void **)genQueryOut, NULL);

    return (status);
}

