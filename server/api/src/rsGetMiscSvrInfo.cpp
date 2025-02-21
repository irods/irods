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
#include "irods/irods_logger.hpp"
#include "irods/rodsVersion.h"
#include "irods/miscServerFunct.hpp"
#include "irods/rsGetMiscSvrInfo.hpp"

#include <nlohmann/json.hpp>
#include <openssl/asn1.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <vector>

using log_api = irods::experimental::log::api;

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
    try {
        if (irods::CS_NEG_USE_SSL == rsComm->negotiation_results && rsComm->ssl_ctx) {
            certinfo_json["ssl_enabled"] = true;

            const auto* cert = SSL_CTX_get0_certificate(rsComm->ssl_ctx);
            if (cert) {
                std::tm tmp_tm;
                std::array<char, 40> timebuf{};
                // Set notAfter
                auto* asn_time_tmp = X509_get0_notAfter(cert);
                if (asn_time_tmp) {
                    const auto success = ASN1_TIME_to_tm(asn_time_tmp, &tmp_tm);
                    if (success) {
                        std::strftime(&timebuf[0], 40, "%F %T %Z", &tmp_tm);
                        certinfo_json["not_after"] = std::string(timebuf.data());
                    }
                    else {
                        log_api::warn("{}: Failure in ASN1_TIME_to_tm for not_after: time parse failure.", __func__);
                    }
                }
                else {
                    log_api::error("{}: Failure in X509_get0_notAfter: nullptr returned.", __func__);
                }

                // Set notBefore
                asn_time_tmp = X509_get0_notBefore(cert);
                if (asn_time_tmp) {
                    const auto success = ASN1_TIME_to_tm(asn_time_tmp, &tmp_tm);
                    if (success) {
                        std::strftime(&timebuf[0], 40, "%F %T %Z", &tmp_tm);
                        certinfo_json["not_before"] = std::string(timebuf.data());
                    }
                    else {
                        log_api::warn("{}: Failure in ASN1_TIME_to_tm for not_before: time parse failure.", __func__);
                    }
                }
                else {
                    log_api::error("{}: Failure in X509_get0_notBefore: nullptr returned.", __func__);
                }

                // Set pubkey
                auto* bio = BIO_new(BIO_s_mem());
                irods::at_scope_exit free_bio{[&bio] { BIO_free(bio); }};
                if (bio) {
                    char* biodata;
                    long biolen;
                    const auto* pubkey = X509_get0_pubkey(cert);
                    if (pubkey) {
                        const auto ec = EVP_PKEY_print_public(bio, pubkey, 0, nullptr);
                        if (ec > 0) {
                            biolen = BIO_get_mem_data(bio, &biodata);
                            if (!biodata) {
                                log_api::error("{}: Failure in BIO_get_mem_data: nullptr returned.", __func__);
                            }
                            else if (std::isspace(biodata[biolen - 1])) {
                                // strip off trailing newline
                                certinfo_json["public_key"] = std::string(biodata, biolen - 1);
                            }
                            else {
                                certinfo_json["public_key"] = std::string(biodata, biolen);
                            }
                        }
                        else {
                            log_api::warn("{}: Failure in EVP_PKEY_print_public with code [{}].", __func__, ec);
                        }
                    }
                    else {
                        log_api::error("{}: Failure in X509_get0_pubkey: nullptr returned.", __func__);
                    }
                    // Set issuer name
                    BIO_reset(bio);
                    const auto* issuer_name = X509_get_issuer_name(cert);
                    if (issuer_name) {
                        if (X509_NAME_print(bio, issuer_name, 0)) {
                            biolen = BIO_get_mem_data(bio, &biodata);
                            certinfo_json["issuer_name"] = std::string(biodata, biolen);
                        }
                        else {
                            log_api::warn("{}: Failure in X509_NAME_print for issuer_name.", __func__);
                        }
                    }
                    else {
                        log_api::error("{}: Failure in X509_get_issuer_name: nullptr returned.", __func__);
                    }

                    // Set subject name
                    BIO_reset(bio);
                    const auto* subject_name = X509_get_subject_name(cert);
                    if (subject_name) {
                        if (X509_NAME_print(bio, subject_name, 0)) {
                            biolen = BIO_get_mem_data(bio, &biodata);
                            certinfo_json["subject_name"] = std::string(biodata, biolen);
                        }
                        else {
                            log_api::warn("{}: Failure in X509_NAME_print for subject_name.", __func__);
                        }
                    }
                    else {
                        log_api::error("{}: Failure in X509_get_subject_name: nullptr returned.", __func__);
                    }
                }
                else {
                    log_api::error("{}: Failed to create new BIO.", __func__);
                }

                // Set signature algorithm
                certinfo_json["signature_algorithm"] = std::string(OBJ_nid2ln(X509_get_signature_nid(cert)));

                // Set subject alternative name
                GENERAL_NAMES* gen_names =
                    static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(cert, NID_subject_alt_name, nullptr, nullptr));
                std::vector<std::string> san_vector;
                if (gen_names) {
                    unsigned char* tempStr;
                    for (int i = 0; i < sk_GENERAL_NAME_num(gen_names); i++) {
                        GENERAL_NAME* gen_name = sk_GENERAL_NAME_value(gen_names, i);
                        if (gen_name->type != GEN_DNS) {
                            log_api::debug("{}: Non-dNSName Subject Alternative Name found with type [{}].",
                                           __func__,
                                           gen_name->type);
                            // TODO(#8244): support more than just dNSName
                            continue;
                        }
                        int len = ASN1_STRING_to_UTF8(&tempStr, gen_name->d.dNSName);
                        if (len > -1) {
                            auto san_str = std::string(reinterpret_cast<char*>(tempStr), len);
                            san_str.insert(0, "DNS:");
                            san_vector.push_back(san_str);
                        }
                        else {
                            log_api::warn("{}: Failure in ASN1_STRING_to_UTF8 with code [{}].", __func__, len);
                        }
                    }
                }
                if (!san_vector.empty()) {
                    certinfo_json["subject_alternative_names"] = san_vector;
                }
                else {
                    log_api::debug("{}: Subject alternative names has no entries.", __func__);
                }
            }
            else {
                log_api::error("{}: Failure in SSL_CTX_get0_certificate: nullptr returned.", __func__);
            }
        }
        else {
            certinfo_json["ssl_enabled"] = false;
        }

        const auto json_str = certinfo_json.dump();
        myOutSvrInfo->certinfo.buf = malloc(json_str.size());
        myOutSvrInfo->certinfo.len = json_str.size();
        memcpy(myOutSvrInfo->certinfo.buf, json_str.c_str(), myOutSvrInfo->certinfo.len);
    }
    catch (const nlohmann::json::exception& e) {
        log_api::error("{}: Failed to serialize SSL/TLS info - {}", __func__, e.what());
    }
    catch (const std::exception& e) {
        log_api::error("{}: Failed while fetching SSL/TLS info - {}", __func__, e.what());
    }

    return 0;
}
