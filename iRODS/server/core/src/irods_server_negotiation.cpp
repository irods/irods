// =-=-=-=-=-=-=-
#include "irods_client_server_negotiation.hpp"
#include "irods_kvp_string_parser.hpp"
#include "irods_server_properties.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "rodsDef.hpp"
#include "index.hpp"
#include "reFuncDefs.hpp"


namespace irods {
/// =-=-=-=-=-=-=-
/// @brief function which manages the TLS and Auth negotiations with the client
error client_server_negotiation_for_server(
    irods::network_object_ptr _ptr,
    std::string&               _result ) {
    // =-=-=-=-=-=-=-
    // manufacture an rei for the applyRule
    ruleExecInfo_t rei;
    memset( ( char* )&rei, 0, sizeof( ruleExecInfo_t ) );

    // =-=-=-=-=-=-=-
    // if it is, then call the pre PEP and get the result
    msParamArray_t params;
    memset( &params, 0, sizeof( params ) );
    int status = applyRuleUpdateParams(
                     "acPreConnect(*OUT)",
                     &params,
                     &rei,
                     NO_SAVE_REI );
    if ( 0 != status ) {
        return ERROR( status, "failed in call to applyRuleUpdateParams" );
    }

    // =-=-=-=-=-=-=-
    // extract the value from the outgoing param to pass out to the operation
    char* rule_result_ptr = 0;
    msParam_t* out_ms_param = getMsParamByLabel( &params, "*OUT" );
    if ( out_ms_param ) {
        rule_result_ptr = reinterpret_cast< char* >( out_ms_param->inOutStruct );

    }
    else {
        return ERROR( SYS_INVALID_INPUT_PARAM, "null out parameter" );

    }

    if ( !rule_result_ptr ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "rule_result is null" );

    }

    std::string rule_result( rule_result_ptr );
    clearMsParamArray( &params, 0 );

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
    strncpy( cs_neg.result_, rule_result.c_str(), MAX_NAME_LEN );
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
        if( err.ok() ) {

            // =-=-=-=-=-=-=-
            // extract the signed SID
            if ( kvp.find( CS_NEG_SID_KW ) != kvp.end() ) {
                std::string svr_sid = kvp[ CS_NEG_SID_KW ];
                if( !svr_sid.empty() ) {
                    // =-=-=-=-=-=-=-
                    // get our SID to compare
                    server_properties& props = server_properties::getInstance();
                    err = props.capture_if_needed();
                    if( !err.ok() ) {
                        return PASS( err );
                    }

                    // =-=-=-=-=-=-=-
                    // sign local SID
                    std::string signed_sid;
                    err = sign_server_sid(
                              signed_sid );
                    if( !err.ok() ) {
                        return PASS( err );
                    }

                    // =-=-=-=-=-=-=-
                    // check SID against our SID
                    if( svr_sid != signed_sid ) {
                        rodsLog(
                            LOG_ERROR,
                            "client-server negotiation SID mismatch [%s] vs [%s]",
                            svr_sid.c_str(),
                            signed_sid.c_str() );
                    } else {
                        // =-=-=-=-=-=-=-
                        // store property that states this is an Agent-Agent connection
                        props.set_property< std::string >( AGENT_CONN_KW, svr_sid );
                    }

                } // if sid is not empty
                else {
                    rodsLog( LOG_ERROR, "client_server_negotiation_for_server - sent SID is empty" );
                }
            }

            // =-=-=-=-=-=-=-
            // check to see if the result string has the SSL negotiation results
            _result = kvp[ CS_NEG_RESULT_KW ];
            if( _result.empty() ) {
                return ERROR(
                           UNMATCHED_KEY_OR_INDEX,
                           "SSL result string missing from response" );
            }

        } else {
            // =-=-=-=-=-=-=-
            // support 4.0 client-server negotiation which did not
            // use key-val pairs
            _result = read_cs_neg->result_;

        }

    } // if result strlen > 0

    // =-=-=-=-=-=-=-
    // if the response is favorable, return success
    if ( CS_NEG_STATUS_SUCCESS == read_cs_neg->status_ ) {
        return SUCCESS();
    }

    // =-=-=-=-=-=-=-
    // else, return a failure
    return ERROR( -1, "failure detected from client" );

} // client_server_negotiation_for_server

}; // namespace irods



