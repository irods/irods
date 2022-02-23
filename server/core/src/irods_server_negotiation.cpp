// =-=-=-=-=-=-=-
#include "irods/irods_client_server_negotiation.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_kvp_string_parser.hpp"
#include "irods/irods_server_properties.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "irods/rodsDef.h"
#include "irods/rsGlobalExtern.hpp"

#include <list>

#include "fmt/format.h"

namespace irods
{
    error check_sent_sid(const std::string& _zone_key)
    {
        if (_zone_key.empty()) {
            return ERROR(SYS_INVALID_INPUT_PARAM, "incoming zone_key is empty");
        }

        // Check local zone keys for a match with the sent zone key.
        try {
            const auto& neg_key = irods::get_server_property<const std::string>(CFG_NEGOTIATION_KEY_KW);
            if (!negotiation_key_is_valid(neg_key)) {
                irods::log(LOG_WARNING, fmt::format(
                    "[{}:{}] - negotiation_key in server_config is invalid",
                    __func__, __LINE__));
            }

            const auto& zone_key = irods::get_server_property<const std::string>(CFG_ZONE_KEY_KW);

            std::string signed_zone_key;
            if (const auto err = sign_server_sid(zone_key, neg_key, signed_zone_key); !err.ok()) {
                return PASS(err);
            }

            if (_zone_key == signed_zone_key) {
                irods::log(LOG_DEBUG8, fmt::format(
                    "[{}:{}] - signed zone_key matches input signed zone_key",
                    __func__, __LINE__));
                return SUCCESS();
            }
        } catch (const irods::exception& e) {
            return irods::error(e);
        }

        // Check zone and negotiation keys defined for remote zones (i.e. federation) for
        // a match.
        for (const auto& entry : remote_SID_key_map) {
            const auto& [zone_key, neg_key] = entry.second;

            std::string signed_zone_key;
            if (const auto err = sign_server_sid(zone_key, neg_key, signed_zone_key); !err.ok()) {
                return PASS(err);
            }

            if (_zone_key == signed_zone_key) {
                return SUCCESS();
            }
        }

        return ERROR(SYS_SIGNED_SID_NOT_MATCHED, "signed zone_keys do not match");
    } // check_sent_sid


/// =-=-=-=-=-=-=-
/// @brief function which manages the TLS and Auth negotiations with the client
    error client_server_negotiation_for_server(
        irods::network_object_ptr _ptr,
        std::string&               _result ) {
        // =-=-=-=-=-=-=-
        // manufacture an rei for the applyRule
        ruleExecInfo_t rei;
        memset( ( char* )&rei, 0, sizeof( ruleExecInfo_t ) );

        std::string rule_result;
        std::list<boost::any> params;
        params.push_back(&rule_result);

        // =-=-=-=-=-=-=-
        // if it is, then call the pre PEP and get the result
        int status = applyRuleWithInOutVars(
                         "acPreConnect",
                         params,
                         &rei );
        if ( 0 != status ) {
            return ERROR( status, "failed in call to applyRuleUpdateParams" );
        }

        // =-=-=-=-=-=-=-
        // check to see if a negotiation was requested
        if ( !do_client_server_negotiation_for_server() ) {
            // =-=-=-=-=-=-=-
            // if it was not but we require SSL then error out
            if ( CS_NEG_REQUIRE == rule_result ) {
                std::stringstream msg;
                msg << "SSL is required by the server but not requested by the client";
                return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );

            }
            else {
                // =-=-=-=-=-=-=-
                // a negotiation was not requested and we do not require SSL - we good
                return SUCCESS();
            }

        }

        // =-=-=-=-=-=-=-
        // pass the PEP result to the client, send CS_NEG_SVR_1_MSG
        irods::cs_neg_t cs_neg;
        cs_neg.status_ = CS_NEG_STATUS_SUCCESS;
        snprintf( cs_neg.result_, sizeof( cs_neg.result_ ), "%s", rule_result.c_str() );
        error err = send_client_server_negotiation_message( _ptr, cs_neg );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << "failed with PEP value of [" << rule_result << "]";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // get the response from CS_NEG_CLI_1_MSG
        boost::shared_ptr< cs_neg_t > read_cs_neg;
        err = read_client_server_negotiation_message( _ptr, read_cs_neg );
        if ( !err.ok() ) {
            return PASS( err );
        }

        // =-=-=-=-=-=-=-
        // get the result from the key val pair
        if ( strlen( read_cs_neg->result_ ) != 0 ) {
            irods::kvp_map_t kvp;
            err = irods::parse_kvp_string(read_cs_neg->result_, kvp);
            if (err.ok()) {
                if (kvp.find(CS_NEG_SID_KW) != kvp.end()) {
                    // If the zone_key is empty, return immediately because the server sent the
                    // appropriate keyword but did not send a value. This indicates that the
                    // other server is able to perform the negotiation but did not send a value
                    // which is bad.
                    const std::string& zone_key = kvp.at(CS_NEG_SID_KW);
                    if (zone_key.empty()) {
                        return ERROR(REMOTE_SERVER_SID_NOT_DEFINED, fmt::format(
                            "[{}:{}] - received empty zone_key", __func__, __LINE__));
                    }

                    // Make sure that the zone_key was signed using the correct negotiation_key
                    // and matches the signed zone_key for this zone. If it does not match, the
                    // server should end communications.
                    if (const auto err = check_sent_sid(zone_key); !err.ok()) {
                        return err;
                    }

                    // Store property that states this is an Agent-Agent connection, as opposed
                    // to a Client-Agent connection.
                    try {
                        irods::set_server_property<std::string>(AGENT_CONN_KW, zone_key);
                    } catch (const irods::exception& e) {
                        return irods::error(e);
                    }
                }

                // check to see if the result string has the SSL negotiation results
                _result = kvp.at(CS_NEG_RESULT_KW);
                if ( _result.empty() ) {
                    return ERROR(UNMATCHED_KEY_OR_INDEX,
                                 "SSL result string missing from response");
                }
            }
            else {
                // support 4.0 client-server negotiation which did not use key-val pairs
                _result = read_cs_neg->result_;
            }
        } // if result strlen > 0

        if ( CS_NEG_REQUIRE == rule_result &&
             CS_NEG_USE_TCP == _result ) {
            return ERROR(
                       SERVER_NEGOTIATION_ERROR,
                       "request to use TCP refused");
        }
        else if ( CS_NEG_STATUS_SUCCESS == read_cs_neg->status_ ) {
            return SUCCESS();
        }

        // =-=-=-=-=-=-=-
        // else, return a failure
        std::stringstream msg;
        msg << "failure detected from client for result ["
            << read_cs_neg->result_
            << "]";
        return ERROR(
                   SERVER_NEGOTIATION_ERROR,
                   msg.str() );

    } // client_server_negotiation_for_server

}; // namespace irods
