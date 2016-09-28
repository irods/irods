/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* miscServerFunct.h - header file for miscServerFunct.c
 */



#ifndef MISC_SERVER_FUNCT_HPP
#define MISC_SERVER_FUNCT_HPP

#include <sys/types.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "rods.h"
#include "rcConnect.h"
#include "fileOpen.h"
#include "dataObjInpOut.h"
#include "dataCopy.h"
#include "rodsConnect.h"

#include "structFileSync.h" /* JMC */

#define MAX_RECON_ERROR_CNT	10

typedef struct PortalTransferInp {
    rsComm_t *rsComm;
    int destFd;
    int srcFd;
    int destRescTypeInx;
    int srcRescTypeInx;
    int threadNum;
    rodsLong_t size;
    rodsLong_t offset;
    rodsLong_t bytesWritten;
    int flags;
    int status;
    dataOprInp_t *dataOprInp;

    int  key_size;
    int  salt_size;
    int  num_hash_rounds;
    char encryption_algorithm[ NAME_LEN ];
    char shared_secret[ NAME_LEN ]; // JMC - shared secret for each portal thread

} portalTransferInp_t;

int
singleL1Copy( rsComm_t *rsComm, dataCopyInp_t *dataCopyInp );
int
svrToSvrConnect( rsComm_t *rsComm, rodsServerHost_t *rodsServerHost );
int
svrToSvrConnect( rsComm_t *rsComm, rodsServerHost_t *rodsServerHost );
int
svrToSvrConnectNoLogin( rsComm_t *rsComm, rodsServerHost_t *rodsServerHost );
int
createSrvPortal( rsComm_t *rsComm, portList_t *thisPortList, int proto );
int
acceptSrvPortal( rsComm_t *rsComm, portList_t *thisPortList );
int
svrPortalPutGet( rsComm_t *rsComm );
void
partialDataPut( portalTransferInp_t *myInput );
void
partialDataGet( portalTransferInp_t *myInput );
int
fillPortalTransferInp( portalTransferInp_t *myInput, rsComm_t *rsComm,
                       int srcFd, int destFd, int destRescTypeInx, int srcRescTypeInx,
                       int threadNum, rodsLong_t size, rodsLong_t offset, int flags );
int
sameHostCopy( rsComm_t *rsComm, dataCopyInp_t *dataCopyInp );
void
sameHostPartialCopy( portalTransferInp_t *myInput );
int
rbudpRemLocCopy( dataCopyInp_t *dataCopyInp );
int
remLocCopy( rsComm_t *rsComm, dataCopyInp_t *dataCopyInp );
void
remToLocPartialCopy( portalTransferInp_t *myInput );
void
locToRemPartialCopy( portalTransferInp_t *myInput );
int
singleRemLocCopy( rsComm_t *rsComm, dataCopyInp_t *dataCopyInp );
int
singleRemToLocCopy( rsComm_t *rsComm, dataCopyInp_t *dataCopyInp );
int
singleLocToRemCopy( rsComm_t *rsComm, dataCopyInp_t *dataCopyInp );
int
isUserPrivileged( rsComm_t *rsComm );
int intNoSupport( ... );
rodsLong_t longNoSupport( ... );
void getZoneServerId( char *zoneName, char *zoneSID );
int
svrPortalPutGetRbudp( rsComm_t *rsComm );
void
reconnManager( rsComm_t *rsComm );
int
svrChkReconnAtReadStart( rsComm_t *rsComm );
int
svrChkReconnAtReadEnd( rsComm_t *rsComm );
int
svrChkReconnAtSendStart( rsComm_t *rsComm );
int
svrChkReconnAtSendEnd( rsComm_t *rsComm );
int
svrSockOpenForInConn( rsComm_t *rsComm, int *portNum, char **addr, int proto );
char *
getLocalSvrAddr();
char *
_getSvrAddr( rodsServerHost_t *rodsServerHost );
char *
getSvrAddr( rodsServerHost_t *rodsServerHost );
int
setLocalSrvAddr( char *outLocalAddr );
int
forkAndExec( char *av[] );
int
setupSrvPortalForParaOpr( rsComm_t *rsComm, dataOprInp_t *dataOprInp,
                          int oprType, portalOprOut_t **portalOprOut );
int
initServiceUser();
int
isServiceUserSet();
int
changeToRootUser();
int
changeToServiceUser();
int
changeToUser( uid_t uid );
int
dropRootPrivilege();
int
checkModArgType( const char *arg );

#ifdef __cplusplus
#include "irods_error.hpp"
#include "irods_plugin_base.hpp"
#include "irods_network_object.hpp"

irods::error readStartupPack(
    irods::network_object_ptr,
    startupPack_t **startupPack,
    struct timeval *tv );

irods::error setRECacheSaltFromEnv();

irods::error get_script_output_single_line(
    const std::string&              script_language,
    const std::string&              script_name,
    const std::vector<std::string>& args,
    std::string&                    output );

irods::error add_global_re_params_to_kvp_for_dynpep(
    keyValPair_t& _kvp );

irods::error get_catalog_service_role( std::string& );
irods::error get_default_rule_plugin_instance(std::string&);
irods::error list_rule_plugin_instances( std::vector< std::string >& );

void applyMetadataFromKVP( rsComm_t *rsComm, dataObjInp_t *dataObjInp);
void applyACLFromKVP( rsComm_t *rsComm, dataObjInp_t *dataObjInp);
#endif // __cplusplus


#endif	/* MISC_SERVER_FUNCT_H */
