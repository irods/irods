


// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_client_server_negotiation.h"

// =-=-=-=-=-=-=-
// irods includes
#include "rodsDef.h"
#include "index.h"
#include "reFuncDefs.h"


namespace eirods {
    /// =-=-=-=-=-=-=-
    /// @brief function which manages the TLS and Auth negotiations with the client
    error client_server_negotiation_for_server( 
        eirods::network_object_ptr _ptr,
        std::string&               _result ) {
        // =-=-=-=-=-=-=-
        // manufacture an rei for the applyRule
        ruleExecInfo_t rei;
        memset ((char*)&rei, 0, sizeof (ruleExecInfo_t));
        
        // =-=-=-=-=-=-=-
        // if it is, then call the pre PEP and get the result
        msParamArray_t params;
        memset( &params, 0, sizeof( params ) );
        int status = applyRuleUpdateParams( 
                         "acPreConnect(*OUT)", 
                         &params, 
                         &rei, 
                         NO_SAVE_REI );
        if( 0 != status ) {
            return ERROR( status, "failed in call to applyRuleUpdateParams" );
        }

        // =-=-=-=-=-=-=-
        // extract the value from the outgoing param to pass out to the operation
        char* rule_result_ptr = 0;
        msParam_t* out_ms_param = getMsParamByLabel( &params, "*OUT" );
        if( out_ms_param ) {
            rule_result_ptr = reinterpret_cast< char* >( out_ms_param->inOutStruct );

        } else {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null out parameter" );

        }

        if( !rule_result_ptr ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "rule_result is null" );

        }

        std::string rule_result( rule_result_ptr );

        // =-=-=-=-=-=-=-
        // check to see if a negoation was requested 
        if( !do_client_server_negotiation() ) {
            // =-=-=-=-=-=-=-
            // if it was not but we require SSL then error out
            if( CS_NEG_REQUIRE == rule_result ) {
                std::stringstream msg;
                msg << "SSL is required by the server but not requested by the client";
                return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );

            } else {
                // =-=-=-=-=-=-=-
                // a negotiation was not requested, bail
                return SUCCESS();
            }

        }

        // =-=-=-=-=-=-=-
        // pass the PEP result to the client, send CS_NEG_SVR_1_MSG
        eirods::cs_neg_t cs_neg;
        cs_neg.status_ = CS_NEG_STATUS_SUCCESS;
        strncpy( cs_neg.result_, rule_result.c_str(), MAX_NAME_LEN );
        error err = send_client_server_negotiation_message( _ptr, cs_neg );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << "failed with PEP value of [" << rule_result << "]";
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // get the response from CS_NEG_CLI_1_MSG
        boost::shared_ptr< cs_neg_t > read_cs_neg;
        err = read_client_server_negotiation_message( _ptr, read_cs_neg );
        if( !err.ok() ) {
            return PASS( err );
        }
        
        // =-=-=-=-=-=-=-
        // get the result from the key val pair
        if( strlen( read_cs_neg->result_ ) != 0 ) {
            _result = read_cs_neg->result_;
        }

        // =-=-=-=-=-=-=-
        // if the response is favorable, return success
        if( CS_NEG_STATUS_SUCCESS == read_cs_neg->status_ ) {
            return SUCCESS();    
        }

        // =-=-=-=-=-=-=-
        // else, return a failure
        return ERROR( -1, "failure detected from client" );

    } // client_server_negotiation_for_server

}; // namespace eirods



