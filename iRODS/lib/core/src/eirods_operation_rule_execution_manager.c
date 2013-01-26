

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
            // build an argument list for applyRule from the gathered op params
            char** char_args = new char*[ _args.size() ];
            for( size_t i = 0; i < _args.size(); ++i ) {
                if( _args[ i ].size() > MAX_NAME_LEN ) {
                    std::stringstream msg;
                    msg << _name;
                    msg << " arg [" << _args[ i ] << "]";
                    msg << " is longer than " << MAX_NAME_LEN;
                    msg << "characters.";
                    log( ERROR( -1, msg.str() ) );
                }

                char_args[ i ] = new char[ MAX_NAME_LEN ];
                rstrcpy( char_args[ i ], _args[ i ].c_str(), MAX_NAME_LEN );

            } // for i

            // =-=-=-=-=-=-=-
            // manufacture an rei for the applyRule
            ruleExecInfo_t rei;
            memset ((char*)&rei, 0, sizeof (ruleExecInfo_t));
            rei.rsComm = _comm;
            rei.uoic   = &_comm->clientUser;
            rei.uoip   = &_comm->proxyUser;

            // =-=-=-=-=-=-=-
            // it does, so execute
            applyRuleArg( const_cast<char*>( _name.c_str() ), char_args, _args.size(), &rei, NO_SAVE_REI );
                          
            // =-=-=-=-=-=-=-
            // clean up arg array
            for( size_t i = 0; i < _args.size(); ++i ) {
                delete [] char_args[ i ];
            }

            delete char_args;

           return SUCCESS();

        } // exec_pre_op

}; // namespace eirods



