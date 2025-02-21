/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsGetMiscSvrInfo.c - server routine that handles the the GetMiscSvrInfo
 * API
 */

/* script generated code */
#include "irods/getMiscSvrInfo.h"
#include "irods/irods_client_server_negotiation.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/irods_log.hpp"
#include "irods/rodsVersion.h"
#include "irods/miscServerFunct.hpp"
#include "irods/rsGetMiscSvrInfo.hpp"

#include <nlohmann/json.hpp>

int
rsGetMiscSvrInfo( rsComm_t *rsComm, miscSvrInfo_t **outSvrInfo ) {
    if ( !rsComm || !outSvrInfo ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    miscSvrInfo_t *myOutSvrInfo;
    char *tmpStr;

    myOutSvrInfo = *outSvrInfo = ( miscSvrInfo_t* )malloc( sizeof( miscSvrInfo_t ) );

    memset( myOutSvrInfo, 0, sizeof( miscSvrInfo_t ) );

    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }


    if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        myOutSvrInfo->serverType = RCAT_ENABLED;
    } else if( irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        myOutSvrInfo->serverType = RCAT_NOT_ENABLED;
    } else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        return SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }

    rstrcpy( myOutSvrInfo->relVersion, RODS_REL_VERSION, NAME_LEN );
    rstrcpy( myOutSvrInfo->apiVersion, RODS_API_VERSION, NAME_LEN );

    const auto& zone_name = irods::get_server_property<const std::string>(irods::KW_CFG_ZONE_NAME);
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();

    }

    snprintf( myOutSvrInfo->rodsZone, NAME_LEN, "%s", zone_name.c_str() );
    if ( ( tmpStr = getenv( SERVER_BOOT_TIME ) ) != NULL ) {
        myOutSvrInfo->serverBootTime = atoi( tmpStr );
    }

    nlohmann::json certinfo_json;
    if(irods::CS_NEG_USE_SSL == rsComm->negotiation_results && rsComm->ssl_ctx) {
        certinfo_json["ssl_enabled"] = true;

        const auto* cert = SSL_CTX_get0_certificate(rsComm->ssl_ctx);
        if(cert) {
            std::tm tmp_tm;
            char timebuf[40] = { 0 };
            // set notAfter
            auto asn_time_tmp = X509_get0_notAfter(cert);
            ASN1_TIME_to_tm(asn_time_tmp, &tmp_tm);
            std::strftime(&timebuf[0], 40, "%F %T %Z", &tmp_tm);
            certinfo_json["notAfter"] = std::string(timebuf);

            // set notBefore
            asn_time_tmp = X509_get0_notBefore(cert);
            ASN1_TIME_to_tm(asn_time_tmp, &tmp_tm);
            std::strftime(&timebuf[0], 40, "%F %T %Z", &tmp_tm);
            certinfo_json["notBefore"] = std::string(timebuf);

            // set pubkey
            auto* bio = BIO_new(BIO_s_mem());
            irods::at_scope_exit free_bio {[&bio] { BIO_free(bio); } };
            char* biodata;
            const auto* pubkey = X509_get0_pubkey(cert);
            EVP_PKEY_print_public(bio, pubkey, 0, NULL);
            const auto biolen = BIO_get_mem_data(bio, &biodata);
            certinfo_json["pubkey"] = std::string(biodata, biolen);
        }

    } else {
        certinfo_json["ssl_enabled"] = false;
    }

    const auto json_str = certinfo_json.dump();
    myOutSvrInfo->certinfo.buf = malloc(json_str.size());
    myOutSvrInfo->certinfo.len = json_str.size();
    memcpy(myOutSvrInfo->certinfo.buf, json_str.c_str(), myOutSvrInfo->certinfo.len);

    return 0;
}
