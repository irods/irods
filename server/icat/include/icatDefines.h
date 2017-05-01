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
   all the rest.  And if you have "delete metadata", you have "modify
   metadata".  The ICAT code generates sql that asks, essentially,
   "does the user have ACCESS_x or better?"

 */
#define ACCESS_NULL                 "null"
#define ACCESS_EXECUTE              "execute"
#define ACCESS_READ_ANNOTATION      "read annotation"
#define ACCESS_READ_SYSTEM_METADATA "read system metadata"
#define ACCESS_READ_METADATA        "read metadata"
#define ACCESS_READ_OBJECT          "read object"
#define ACCESS_WRITE_ANNOTATION     "write annotation"
#define ACCESS_CREATE_METADATA      "create metadata"
#define ACCESS_MODIFY_METADATA      "modify metadata"
#define ACCESS_DELETE_METADATA      "delete metadata"
#define ACCESS_ADMINISTER_OBJECT    "administer object"
#define ACCESS_CREATE_OBJECT        "create object"
#define ACCESS_MODIFY_OBJECT        "modify object"
#define ACCESS_DELETE_OBJECT        "delete object"
#define ACCESS_CREATE_TOKEN         "create token"
#define ACCESS_DELETE_TOKEN         "delete token"
#define ACCESS_CURATE               "curate"
#define ACCESS_OWN                  "own"

#define ACCESS_INHERIT              "inherit"
#define ACCESS_NO_INHERIT           "noinherit"


// The following are the Auditing action values

#define AU_ACCESS_GRANTED                      1000

#define AU_REGISTER_DATA_OBJ                   2010
#define AU_REGISTER_DATA_REPLICA               2011
#define AU_UNREGISTER_DATA_OBJ                 2012

#define AU_REGISTER_DELAYED_RULE               2020
#define AU_MODIFY_DELAYED_RULE                 2021
#define AU_DELETE_DELAYED_RULE                 2022

#define AU_REGISTER_RESOURCE                   2030
#define AU_DELETE_RESOURCE                     2031

#define AU_DELETE_USER_RE                      2040

#define AU_REGISTER_COLL_BY_ADMIN              2050
#define AU_REGISTER_COLL                       2051

#define AU_DELETE_COLL_BY_ADMIN                2060
#define AU_DELETE_COLL                         2061
#define AU_DELETE_ZONE                         2062

#define AU_REGISTER_ZONE                       2064

#define AU_MOD_USER_NAME                       2070
#define AU_MOD_USER_TYPE                       2071
#define AU_MOD_USER_ZONE                       2072
#define AU_MOD_USER_DN                         2073  // no longer used
#define AU_MOD_USER_INFO                       2074
#define AU_MOD_USER_COMMENT                    2075
#define AU_MOD_USER_PASSWORD                   2076

#define AU_ADD_USER_AUTH_NAME                  2077
#define AU_DELETE_USER_AUTH_NAME               2078

#define AU_MOD_GROUP                           2080
#define AU_MOD_RESC                            2090
#define AU_MOD_RESC_FREE_SPACE                 2091
#define AU_MOD_RESC_GROUP                      2092
#define AU_MOD_ZONE                            2093

#define AU_REGISTER_USER_RE                    2100
#define AU_ADD_AVU_METADATA                    2110
#define AU_DELETE_AVU_METADATA                 2111
#define AU_COPY_AVU_METADATA                   2112
#define AU_ADD_AVU_WILD_METADATA               2113

#define AU_MOD_ACCESS_CONTROL_OBJ              2120
#define AU_MOD_ACCESS_CONTROL_COLL             2121
#define AU_MOD_ACCESS_CONTROL_COLL_RECURSIVE   2122
#define AU_MOD_ACCESS_CONTROL_RESOURCE         2123

#define AU_RENAME_DATA_OBJ                     2130
#define AU_RENAME_COLLECTION                   2131

#define AU_MOVE_DATA_OBJ                       2140
#define AU_MOVE_COLL                           2141

#define AU_REG_TOKEN                           2150
#define AU_DEL_TOKEN                           2151

#define AU_ADD_CHILD_RESOURCE                  2160
#define AU_DEL_CHILD_RESOURCE                  2161

#define AU_CREATE_TICKET                       2170
#define AU_MOD_TICKET                          2171
#define AU_DELETE_TICKET                       2172
#define AU_USE_TICKET                          2173



#endif // ICAT_DEFINES_H__
