#ifndef CLIENT_HINTS_HPP
#define CLIENT_HINTS_HPP

// =-=-=-=-=-=-=-
// irods includes
#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"

#define CLIENT_HINTS_AN 10215

#ifdef RODS_SERVER
#define RS_CLIENT_HINTS rsClientHints
int rsClientHints(
    rsComm_t*,      // server comm ptr
    bytesBuf_t** ); // json response
#else
#define RS_CLIENT_HINTS NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
// =-=-=-=-=-=-=-
// prototype for client
int rcClientHints(
    rcComm_t*,      // server comm ptr
    bytesBuf_t** ); // json response
#ifdef __cplusplus
}
#endif



#endif //  CLIENT_HINTS_HPP




