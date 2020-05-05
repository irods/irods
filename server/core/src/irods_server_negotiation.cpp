// =-=-=-=-=-=-=-
#include "irods_client_server_negotiation.hpp"
#include "irods_configuration_keywords.hpp"
#include "irods_kvp_string_parser.hpp"
#include "irods_server_properties.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "rodsDef.h"
#include "rsGlobalExtern.hpp"

#include <list>

namespace irods {
/// =-=-=-=-=-=-=-
/// @brief check the incoming signed SID against all locals SIDs
    error check_sent_sid(
        const std::string  _in_sid ) {
        // =-=-=-=-=-=-=-
        // check incoming params
        if ( _in_sid.empty() ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "incoming SID is empty" );
        }

        try {
            // =-=-=-=-=-=-=-
            // get the agent key
            const auto& encr_key = irods::get_server_property<const std::string>(CFG_NEGOTIATION_KEY_KW);

            // =-=-=-=-=-=-=-
            // start with local SID
            const auto& svr_sid = irods::get_server_property<const std::string>(CFG_ZONE_KEY_KW);

            // =-=-=-=-=-=-=-
            // sign SID
            std::string signed_sid;
            irods::error err = sign_server_sid(
                    svr_sid,
                    encr_key,
                    signed_sid );
            if ( !err.ok() ) {
                return PASS( err );
            }

            // =-=-=-=-=-=-=-
            // if it is a match, were good
            if ( _in_sid == signed_sid ) {
                return SUCCESS();
            }
        } catch ( const irods::exception& e ) {
            return irods::error(e);
        }

        // =-=-=-=-=-=-=-
        // if not, check against all remote zone SIDs and keys
        irods::lookup_table <
        std::pair <
        std::string,
            std::string > >::iterator itr = remote_SID_key_map.begin();
        for ( ; itr != remote_SID_key_map.end(); ++itr ) {
            const std::pair<std::string, std::string>& entry = itr->second;

            // =-=-=-=-=-=-=-
            // sign SID
            std::string signed_sid;
            irods::error err = sign_server_sid(
                      entry.first,
                      entry.second,
                      signed_sid );
            if ( !err.ok() ) {
                return PASS( err );
            }

            // =-=-=-=-=-=-=-
            // basic string compare
            if ( _in_sid == signed_sid ) {
                return SUCCESS();
            }

        } // for itr

        return ERROR(
                   SYS_SIGNED_SID_NOT_MATCHED,
                   "signed SID was not matched" );

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
        // check to see if a negoation was requested
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
                // a negotiation was not requested, bail
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
            err = irods::parse_kvp_string(
                      read_cs_neg->result_,
                      kvp );
            if ( err.ok() ) {

                // =-=-=-=-=-=-=-
                // extract the signed SID
                if ( kvp.find( CS_NEG_SID_KW ) != kvp.end() ) {
                    std::string svr_sid = kvp[ CS_NEG_SID_KW ];
                    if ( !svr_sid.empty() ) {
                        // =-=-=-=-=-=-=-
                        // check SID against our SIDs
                        err = check_sent_sid(
                                  svr_sid );
                        if ( !err.ok() ) {
                            rodsLog(
                                LOG_DEBUG,
                                "CS_NEG\n%s",
                                PASS( err ).result().c_str() );
                        }
                        else {
                            // =-=-=-=-=-=-=-
                            // store property that states this is an
                            // Agent-Agent connection
                            try {
                                irods::set_server_property<std::string>(AGENT_CONN_KW, svr_sid);
                            } catch ( const irods::exception& e ) {
                                return irods::error(e);
                            }
                        }

                    } // if sid is not empty
                    else {
                        rodsLog(
                            LOG_WARNING,
                            "CS_NEG :: %s - sent SID is empty",
                            __FUNCTION__ );
                    }
                }

                // =-=-=-=-=-=-=-
                // check to see if the result string has the SSL negotiation results
                _result = kvp[ CS_NEG_RESULT_KW ];
                if ( _result.empty() ) {
                    return ERROR(
                               UNMATCHED_KEY_OR_INDEX,
                               "SSL result string missing from response" );
                }

            }
            else {
                // =-=-=-=-=-=-=-
                // support 4.0 client-server negotiation which did not
                // use key-val pairs
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
