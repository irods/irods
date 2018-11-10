/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rodsClient.h - common header file for rods client
 */

#ifndef RODS_CLIENT_H__
#define RODS_CLIENT_H__

#include "rods.h"
#include "apiHeaderAll.h"

/* Struct used to monitor transfer progress in getUtil and putUtil functions.*/
typedef struct TransferProgress {
    rodsLong_t bytesReceived;
    rodsLong_t bytesExpected;
    char currentFilePath[MAX_NAME_LEN];
} xferProgress_t;

void init_client_api_table(void);

#endif  /* RODS_CLIENT_H__ */
