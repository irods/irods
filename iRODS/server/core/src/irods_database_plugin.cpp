// =-=-=-=-=-=-=-
// My Includes
#include "irods_database_plugin.hpp"
#include "irods_load_plugin.hpp"
#include "irods_stacktrace.hpp"
#include "irods_operation_rule_execution_manager_base.hpp"

// =-=-=-=-=-=-=-
// STL Includes
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>

// =-=-=-=-=-=-=-
// dlopen, etc
#include <dlfcn.h>

namespace irods {

// =-=-=-=-=-=-=-
// public - ctor
    database::database(
        const std::string& _inst,
        const std::string& _ctx ) :
        plugin_base(
            _inst,
            _ctx ),
        start_operation_( irods::database::default_start_operation ),
        stop_operation_( irods::database::default_stop_operation ) {
    } // ctor

// =-=-=-=-=-=-=-
// public - dtor
    database::~database( ) {

    } // dtor

// =-=-=-=-=-=-=-
// public - cctor
    database::database(
        const database& _rhs ) :
        plugin_base( _rhs ) {
        start_operation_    = _rhs.start_operation_;
        stop_operation_     = _rhs.stop_operation_;
        operations_         = _rhs.operations_;
        ops_for_delay_load_ = _rhs.ops_for_delay_load_;

        if ( properties_.size() > 0 ) {
            std::cout << "[!]\tdatabase cctor - properties map is not empty."
                      << __FILE__ << ":" << __LINE__ << std::endl;
        }
        properties_ = _rhs.properties_; // NOTE:: memory leak repaving old containers?
    } // cctor

// =-=-=-=-=-=-=-
// public - assignment
    database& database::operator=(
        const database& _rhs ) {
        if ( &_rhs == this ) {
            return *this;
        }

        plugin_base::operator=( _rhs );

        operations_         = _rhs.operations_;
        ops_for_delay_load_ = _rhs.ops_for_delay_load_;

        if ( properties_.size() > 0 ) {
            std::cout << "[!]\tdatabase cctor - properties map is not empty."
                      << __FILE__ << ":" << __LINE__ << std::endl;
        }

        properties_ = _rhs.properties_; // NOTE:: memory leak repaving old containers?

        return *this;

    } // operator=

// =-=-=-=-=-=-=-
// public - function which pulls all of the symbols out of the shared object and
//          associates them with their keys in the operations table
    error database::delay_load(
        void* _handle ) {
        // =-=-=-=-=-=-=-
        // check params
        if ( ! _handle ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "void handle pointer" );
        }

        // =-=-=-=-=-=-=-
        // check if we need to load a start function
        if ( !start_opr_name_.empty() ) {
            dlerror();
            database_maintenance_operation start_op = reinterpret_cast< database_maintenance_operation >(
                        dlsym( _handle, start_opr_name_.c_str() ) );
            if ( !start_op ) {
                std::stringstream msg;
                msg  << "failed to load start function ["
                     << start_opr_name_ << "]";
                return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
            }
            else {
                start_operation_ = start_op;
            }
        } // if load start


        // =-=-=-=-=-=-=-
        // check if we need to load a stop function
        if ( !stop_opr_name_.empty() ) {
            dlerror();
            database_maintenance_operation stop_op = reinterpret_cast< database_maintenance_operation >(
                        dlsym( _handle, stop_opr_name_.c_str() ) );
            if ( !stop_op ) {
                std::stringstream msg;
                msg << "failed to load stop function ["
                    << stop_opr_name_ << "]";
                return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
            }
            else {
                stop_operation_ = stop_op;
            }
        } // if load stop

        // =-=-=-=-=-=-=-
        // iterate over list and load function. then add it to the map via wrapper functor
        std::vector< std::pair< std::string, std::string > >::iterator itr = ops_for_delay_load_.begin();
        for ( ; itr != ops_for_delay_load_.end(); ++itr ) {
            // =-=-=-=-=-=-=-
            // cache values in obvious variables
            std::string& key = itr->first;
            std::string& fcn = itr->second;

            // =-=-=-=-=-=-=-
            // check key and fcn name before trying to load
            if ( key.empty() ) {
                std::cout << "[!]\tirods::database::delay_load - empty op key for ["
                          << fcn << "], skipping." << std::endl;
                continue;
            }

            // =-=-=-=-=-=-=-
            // check key and fcn name before trying to load
            if ( fcn.empty() ) {
                std::cout << "[!]\tirods::database::delay_load - empty function name for ["
                          << key << "], skipping." << std::endl;
                continue;
            }

            // =-=-=-=-=-=-=-
            // call dlsym to load and check results
            dlerror();
            plugin_operation res_op_ptr = reinterpret_cast< plugin_operation >( dlsym( _handle, fcn.c_str() ) );
            if ( !res_op_ptr ) {
                std::cout << "[!]\tirods::database::delay_load - failed to load ["
                          << fcn << "].  error - " << dlerror() << std::endl;
                continue;
            }

            // =-=-=-=-=-=-=-
            // add the operation via a wrapper to the operation map
#ifdef LINK_NO_OP_RE_MGR
            oper_rule_exec_mgr_ptr rex_mgr(
                new operation_rule_execution_manager_no_op(
                    instance_name_, key ) );
#else
            oper_rule_exec_mgr_ptr rex_mgr(
                new operation_rule_execution_manager(
                    instance_name_, key ) );
#endif
            // =-=-=-=-=-=-=-
            // add the operation via a wrapper to the operation map
            operations_[ key ] = operation_wrapper<void>(
                                     rex_mgr,
                                     instance_name_,
                                     key,
                                     res_op_ptr );
        } // for itr

        return SUCCESS();

    } // delay_load

// =-=-=-=-=-=-=-
// public - set a name for the developer provided start op
    void database::set_start_operation(
        const std::string& _op ) {
        start_opr_name_ = _op;
    } // database::set_start_operation

// =-=-=-=-=-=-=-
// public - set a name for the developer provided stop op
    void database::set_stop_operation(
        const std::string& _op ) {
        stop_opr_name_ = _op;
    } // database::set_stop_operation

// END database
// =-=-=-=-=-=-=-

// =-=-=-=-=-=-=-
// function to load and return an initialized database plugin
    error load_database_plugin(
        database_ptr&      _plugin,
        const std::string& _plugin_name,
        const std::string& _inst_name,
        const std::string& _context ) {

        // =-=-=-=-=-=-=-
        // call generic plugin loader
        database* db = 0;
        error ret = load_plugin< database >(
                        db,
                        _plugin_name,
                        PLUGIN_TYPE_DATABASE,
                        _inst_name,
                        _context );
        if ( ret.ok() && db ) {
            _plugin.reset( db );
            return SUCCESS();

        }

        return PASS( ret );

    } // load_database_plugin

}; // namespace irods



