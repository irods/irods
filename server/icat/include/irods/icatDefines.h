/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/****************************************************************************

   This file contains all the constant definitions used by ICAT

*****************************************************************************/

#ifndef ICAT_DEFINES_H__
#define ICAT_DEFINES_H__

#define   MAX_NUM_OF_SELECT_ITEMS                  30
#define   MAX_NUM_OF_CONCURRENT_STMTS              50
#define   MAX_NUM_OF_COLS_IN_TABLE                 50
#define   MAX_SQL_SIZE                             4000
#define   MAX_SQL_SIZE_GENERAL_QUERY               16000

#define MAX_INTEGER_SIZE 40  /* ??, for now */

#define DB_USERNAME_LEN       64
#define DB_PASSWORD_LEN       64
#define DB_TYPENAME_LEN       64

#define DB_TYPE_POSTGRES 1
#define DB_TYPE_ORACLE   2
#define DB_TYPE_MYSQL    3

#define TICKET_TYPE_DATA "data"
#define TICKET_TYPE_COLL "collection"

/*
   These are the access permissions known to the system, listed in
   order.  The defines are here to make it clear what these are and
   where in the code they are being used.

   These, and their integer values, are defined in the R_TOKN_MAIN
   table.

   Having a particular access permission implies that the user has
   all of the lower ones.  For example, if you have "own", you have
   all the rest.  And if you have "delete_metadata", you have
   "modify_metadata".  The ICAT code generates sql that asks,
   essentially, "does the user have ACCESS_x or better?"

 */
#define ACCESS_NULL                                "null"
#define ACCESS_EXECUTE                             "execute"
#define ACCESS_READ_ANNOTATION                     "read_annotation"
#define ACCESS_READ_SYSTEM_METADATA                "read_system_metadata"
#define ACCESS_READ_METADATA                       "read_metadata"
#define ACCESS_READ_OBJECT                         "read_object"
#define ACCESS_WRITE_ANNOTATION                    "write_annotation"
#define ACCESS_CREATE_METADATA                     "create_metadata"
#define ACCESS_MODIFY_METADATA                     "modify_metadata"
#define ACCESS_DELETE_METADATA                     "delete_metadata"
#define ACCESS_ADMINISTER_OBJECT                   "administer_object"
#define ACCESS_CREATE_OBJECT                       "create_object"
#define ACCESS_MODIFY_OBJECT                       "modify_object"
#define ACCESS_DELETE_OBJECT                       "delete_object"
#define ACCESS_CREATE_TOKEN                        "create_token"
#define ACCESS_DELETE_TOKEN                        "delete_token"
#define ACCESS_CURATE                              "curate"
#define ACCESS_OWN                                 "own"

#define ACCESS_INHERIT                             "inherit"
#define ACCESS_NO_INHERIT                          "noinherit"

#endif // ICAT_DEFINES_H__
