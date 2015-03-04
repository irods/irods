#ifndef SERVER_REPORT_HPP
#define SERVER_REPORT_HPP

// =-=-=-=-=-=-=-
// irods includes
#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"

#define SERVER_REPORT_AN 10204

#ifdef RODS_SERVER
#define RS_SERVER_REPORT rsServerReport
int rsServerReport(
    rsComm_t*,      // server comm ptr
    bytesBuf_t** ); // json response
#else
#define RS_SERVER_REPORT NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
// =-=-=-=-=-=-=-
// prototype for client
int rcServerReport(
    rcComm_t*,      // server comm ptr
    bytesBuf_t** ); // json response
#ifdef __cplusplus
}
#endif



#endif //  SERVER_REPORT_HPP




