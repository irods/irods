

// =-=-=-=-=-=-=-
// irods includes
#include "index.h"
#include "reFuncDefs.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_operation_rule_execution_manager.h"

namespace eirods {
        // =-=-=-=-=-=-=-
        // public - Constructor
        operation_rule_execution_manager::operation_rule_execution_manager( 
                                          const std::string& _inst_name,
                                          const std::string& _op_name ) {
            rule_name_ = _inst_name + "_" + _op_name;

        } // ctor

        // =-=-=-=-=-=-=-
        // public - execute rule for pre operation
        error operation_rule_execution_manager::exec_pre_op( rsComm_t*                         _comm, 
                                                             const std::vector< std::string >& _args,
                                                             std::string&                      _res ) {
            // =-=-=-=-=-=-=-
            // manufacture pre rule name
            std::string pre_name = rule_name_ + "_pre";

            // =-=-=-=-=-=-=-
            // execute the rule
            return exec_op( pre_name, _comm, _args, _res ); 

        } // exec_post_op

        // =-=-=-=-=-=-=-
        // public - execute rule for post operation
        error operation_rule_execution_manager::exec_post_op( rsComm_t*                         _comm, 
                                                              const std::vector< std::string >& _args,
                                                              std::string&                      _res ) {
            // =-=-=-=-=-=-=-
            // manufacture pre rule name
            std::string post_name = rule_name_ + "_post";

            // =-=-=-=-=-=-=-
            // execute the rule
            return exec_op( post_name, _comm, _args, _res ); 

        } // exec_post_op

        // =-=-=-=-=-=-=-
        // private - execute rule for pre operation
        error operation_rule_execution_manager::exec_op( const std::string&                _name,
                                                         rsComm_t*                         _comm, 
                                                         const std::vector< std::string >& _args,
                                                         std::string&                      _res ) {
            // =-=-=-=-=-=-=-
            // check comm ptr
            if( !_comm ) {
                std::stringstream msg;
                msg << _name;
                msg << " comm pointer is null";
                return ERROR( -1, msg.str() );
            }

            // =-=-=-=-=-=-=-
            // determine if rule exists
            RuleIndexListNode* re_node = 0;
            if( findNextRule2( const_cast<char*>( _name.c_str() ), 0, &re_node ) < 0 ) {
                return ERROR( -1, "no rule found" );
            }
            
            // =-=-=-=-=-=-=-
            // check that args are not larger than max args for rules
            if( _args.size() > MAX_NUM_OF_ARGS_IN_ACTION ) {
                std::stringstream msg;
                msg << _name;
                msg << " arg vector is longer than ";
                msg << MAX_NUM_OF_ARGS_IN_ACTION;
                return ERROR( -1, msg.str() );
            }

            // =-=-=-=-=-=-=-
            // manufacture an rei for the applyRule
            ruleExecInfo_t rei;
            memset ((char*)&rei, 0, sizeof (ruleExecInfo_t));
            rei.rsComm = _comm;
            rei.uoic   = &_comm->clientUser;
            rei.uoip   = &_comm->proxyUser;

            // =-=-=-=-=-=-=-
            // build an argument list for applyRule from the gathered op params
            msParamArray_t params;
            memset( &params, 0, sizeof( params ) );

            int            label_index = 0;
            std::string    label_base( "*ARG" );
            std::string    arg_string( "(" );
            for( size_t i = 0; i < _args.size(); ++i ) {
                std::stringstream idx; 
                idx << label_index++; 
                std::string label = label_base + idx.str();
                
                addMsParamToArray( &params, const_cast<char*>( label.c_str() ), STR_MS_T, const_cast<char*>( _args[ i ].c_str() ), NULL, 0 );
               
                arg_string += label + ",";
                 
            } // for i
                
            // =-=-=-=-=-=-=-
            // add the output parameter     
            arg_string += "*OUT";
            char out_param[ MAX_NAME_LEN ] = {"EMPTY_PARAM"};
            addMsParamToArray( &params, "*OUT", STR_MS_T, out_param, NULL, 0 );

            // =-=-=-=-=-=-=-
            // rule exists, param array is build.  call the rule.
            std::string arg_name = _name + arg_string + ")";
            int ret = applyRuleUpdateParams( const_cast<char*>( arg_name.c_str() ), &params, &rei, NO_SAVE_REI );
            if( 0 != ret ) {
                return ERROR( ret, "exec_op - failed in call to applyRuleUpdateParams" );
            }

            // =-=-=-=-=-=-=-
            // extract the value from the outgoing param to pass out to the operation
            msParam_t* out_ms_param = getMsParamByLabel( &params, "*OUT" );
            if( out_ms_param ) {
                _res = reinterpret_cast< char* >( out_ms_param->inOutStruct ); 
            } else {
                return ERROR( -1, "exec_op - null out parameter" );    
            }

            return SUCCESS();

        } // exec_op

}; // namespace eirods



