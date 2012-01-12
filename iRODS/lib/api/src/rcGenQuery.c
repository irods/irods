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

