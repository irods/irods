#ifndef IES_CLIENT_HINTS_HPP
#define IES_CLIENT_HINTS_HPP

// =-=-=-=-=-=-=-
// irods includes
#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"

#define IES_CLIENT_HINTS_AN 10216

#ifdef RODS_SERVER
#define RS_IES_CLIENT_HINTS rsClientHints
int rsIESClientHints(
    rsComm_t*,      // server comm ptr
    bytesBuf_t** ); // json response
#else
#define RS_IES_CLIENT_HINTS NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
// =-=-=-=-=-=-=-
// prototype for client
int rcIESClientHints(
    rcComm_t*,      // server comm ptr
    bytesBuf_t** ); // json response
#ifdef __cplusplus
}
#endif



#endif //  IES_CLIENT_HINTS_HPP




