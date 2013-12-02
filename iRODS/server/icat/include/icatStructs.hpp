/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/***************************************************************************

   This file contains all the structuresdefinitions used by ICAT.

****************************************************************************/

#ifndef ICAT_STRUCTS_H
#define ICAT_STRUCTS_H

#include "icatDefines.h"

typedef struct
{
  int status;
  void*   stmtPtr;                            /* internal db statemnt handle */
  int     numOfCols;                          /* number of result columns */
  char    *resultColName[MAX_NUM_OF_SELECT_ITEMS];  /* column names */
  int     selectColIds[MAX_NUM_OF_SELECT_ITEMS];  /* rods-id to column in the
                                                     result (unused, so far) */
  char    *resultValue[MAX_NUM_OF_SELECT_ITEMS];  /* pointer to data area */
} icatStmtStrct;



typedef struct {
  int         status;
  void*       environPtr;       /* internal db environment handle */
  void*       connectPtr;       /* internal db connection handle */
  icatStmtStrct* stmtPtr[MAX_NUM_OF_CONCURRENT_STMTS];  /* statement handles */
  char databaseUsername[DB_USERNAME_LEN];  /* username for accessing the db */
  char databasePassword[DB_PASSWORD_LEN];  /* password for accessing the db */
  int         databaseType;     /* DB type, DB_TYPE_POSTGRES, etc */
}icatSessionStruct;


#endif /* ICAT_STRUCTS_H */
