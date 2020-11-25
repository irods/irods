#ifndef RODS_CLIENT_H__
#define RODS_CLIENT_H__

#include "rods.h"
#include "apiHeaderAll.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Struct used to monitor transfer progress in getUtil and putUtil functions.*/
typedef struct TransferProgress {
    rodsLong_t bytesReceived;
    rodsLong_t bytesExpected;
    char currentFilePath[MAX_NAME_LEN];
} xferProgress_t;

void init_client_api_table(void) __attribute__((deprecated("Use load_client_api_plugins instead")));

void load_client_api_plugins();

#ifdef __cplusplus
} // extern "C"
#endif

#endif  /* RODS_CLIENT_H__ */
