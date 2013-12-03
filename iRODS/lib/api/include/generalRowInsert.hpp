/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* generalRowInsert.h

   This client/server call is used to insert rows into certain
   special-purpose tables.  See the rs file for the current list.
   Admin only.  Also see generalRowPurge.
 */

#ifndef GENERAL_ROW_INSERT_HPP
#define GENERAL_ROW_INSERT_HPP

/* This is a Metadata type API call */


#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "initServer.hpp"
#include "icatDefines.hpp"

typedef struct {
   char *tableName;
   char *arg1;
   char *arg2;
   char *arg3;
   char *arg4;
   char *arg5;
   char *arg6;
   char *arg7;
   char *arg8;
   char *arg9;
  char *arg10;
} generalRowInsertInp_t;
    
#define generalRowInsertInp_PI "str *tableName; str *arg1; str *arg2; str *arg3; str *arg4; str *arg5; str *arg6; str *arg7;  str *arg8;  str *arg9;"

#ifdef  __cplusplus
extern "C" {
#endif

#if defined(RODS_SERVER)
#define RS_GENERAL_ROW_INSERT rsGeneralRowInsert
/* prototype for the server handler */
int
rsGeneralRowInsert (rsComm_t *rsComm, generalRowInsertInp_t *generalRowInsertInp );

int
_rsGeneralRowInsert (rsComm_t *rsComm, generalRowInsertInp_t *generalRowInsertInp );
#else
#define RS_GENERAL_ROW_INSERT NULL
#endif

/* prototype for the client call */
int
rcGeneralRowInsert (rcComm_t *conn, generalRowInsertInp_t *generalRowInsertInp);

#ifdef  __cplusplus
}
#endif

#endif	/* GENERAL_ROW_INSERT_H */
