/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rodsGeneralUpdate.h - common header file for the generalized update 
   (insert or delete) input.
 */


#ifndef RODS_GENERAL_UPDATE_H
#define RODS_GENERAL_UPDATE_H

#include "objInfo.h"

#include "rodsGenQuery.h"

/* Unusual values since don't want caller to accidentally select wrong call: */
#define GENERAL_UPDATE_INSERT 23451
#define GENERAL_UPDATE_DELETE 23452

/* Use this value to use the next sequence value in a column (for an id) */
#define UPDATE_NEXT_SEQ_VALUE "update_next_sequence_value"

/* Use this value to insert the current time into a column*/
#define UPDATE_NOW_TIME "update_now_time"

#define GeneralUpdateInp_PI "int type; struct InxValPair_PI;"
typedef struct GeneralUpdateInp {
   int type;  /* GEN_UPDATE_INSERT or DELETE */
   inxValPair_t values;  /* Column IDs (from rodsGenQuery.h) and values */
} generalUpdateInp_t;


#endif	/* RODS_GENERAL_UPDATE_H */
