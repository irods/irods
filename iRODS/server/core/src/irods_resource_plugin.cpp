// =-=-=-=-=-=-=-
// My Includes
#include "irods_resource_plugin.hpp"
#include "irods_resource_plugin_impostor.hpp"
#include "irods_load_plugin.hpp"
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
    resource::resource(
        const std::string& _inst,
        const std::string& _ctx ) :
        plugin_base(
            _inst,
            _ctx ),
        start_operation_( default_start_operation ),
        stop_operation_( default_stop_operation ) {
    } // ctor

// =-=-=-=-=-=-=-
// public - dtor
    resource::~resource( ) {

    } // dtor

// =-=-=-=-=-=-=-
// public - cctor
    resource::resource( const resource& _rhs ) :
        plugin_base( _rhs ) {
        children_           = _rhs.children_;
        start_operation_    = _rhs.start_operation_;
        stop_operation_     = _rhs.stop_operation_;
        operations_         = _rhs.operations_;
        ops_for_delay_load_ = _rhs.ops_for_delay_load_;

        if ( properties_.size() > 0 ) {
            std::cout << "[!]\tresource cctor - properties map is not empty."
                      << __FILE__ << ":" << __LINE__ << std::endl;
        }
        properties_ = _rhs.properties_; // NOTE:: memory leak repaving old containers?
    } // cctor

// =-=-=-=-=-=-=-
// public - assignment
    resource& resource::operator=( const resource& _rhs ) {
        if ( &_rhs == this ) {
            return *this;
        }

        plugin_base::operator=( _rhs );

        children_           = _rhs.children_;
        operations_         = _rhs.operations_;
        ops_for_delay_load_ = _rhs.ops_for_delay_load_;

        if ( properties_.size() > 0 ) {
            std::cout << "[!]\tresource cctor - properties map is not empty."
                      << __FILE__ << ":" << __LINE__ << std::endl;
        }
        properties_ = _rhs.properties_; // NOTE:: memory leak repaving old containers?

        return *this;
    } // operator=

// =-=-=-=-=-=-=-
// public - function which pulls all of the symbols out of the shared object and
//          associates them with their keys in the operations table
    error resource::delay_load( void* _handle ) {
        // =-=-=-=-=-=-=-
        // check params
        if ( ! _handle ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "void handle pointer" );
        }

        // =-=-=-=-=-=-=-
        // check if we need to load a start function
        if ( !start_opr_name_.empty() ) {
            dlerror();
            resource_maintenance_operation start_op = reinterpret_cast< resource_maintenance_operation >(
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
            resource_maintenance_operation stop_op = reinterpret_cast< resource_maintenance_operation >(
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
                std::cout << "[!]\tirods::resource::delay_load - empty op key for ["
                          << fcn << "], skipping." << std::endl;
                continue;
            }

            // =-=-=-=-=-=-=-
            // check key and fcn name before trying to load
            if ( fcn.empty() ) {
                std::cout << "[!]\tirods::resource::delay_load - empty function name for ["
                          << key << "], skipping." << std::endl;
                continue;
            }

            // =-=-=-=-=-=-=-
            // call dlsym to load and check results
            dlerror();
            plugin_operation res_op_ptr = reinterpret_cast< plugin_operation >( dlsym( _handle, fcn.c_str() ) );
            if ( !res_op_ptr ) {
                std::cout << "[!]\tirods::resource::delay_load - failed to load ["
                          << fcn << "].  error - " << dlerror() << std::endl;
                continue;
            }

            // =-=-=-=-=-=-=-
            // add the operation via a wrapper to the operation map
            oper_rule_exec_mgr_ptr rex_mgr(
                new operation_rule_execution_manager(
                    instance_name_, key ) );
            operations_[ key ] = operation_wrapper<void>(
                                     rex_mgr,
                                     instance_name_,
                                     key,
                                     res_op_ptr );
        } // for itr

        return SUCCESS();

    } // delay_load

// =-=-=-=-=-=-=-
// public - add a child resource to this resource.  this is virtual in case a developer wants to
//          do something fancier.
    error resource::add_child( const std::string& _name, const std::string& _data, resource_ptr _resc ) {
        // =-=-=-=-=-=-=-
        // check params
        if ( _name.empty() ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "empty name" );
        }

        if ( 0 == _resc.get() ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null resource pointer" );
        }

        // =-=-=-=-=-=-=-
        // add resource and data to the table
        children_[ _name ] = std::pair< std::string, resource_ptr >( _data, _resc );

        return SUCCESS();

    } // add_child

// =-=-=-=-=-=-=-
// public - remove a child resource to this resource.  this is virtual in case a developer wants to
//          do something fancier.
    error resource::remove_child( const std::string& _name ) {
        // =-=-=-=-=-=-=-
        // check params
#ifdef DEBUG
        if ( _name.empty() ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "empty name" );
        }
#endif

        // =-=-=-=-=-=-=-
        // if an entry exists, erase it otherwise issue a warning.
        if ( children_.has_entry( _name ) ) {
            children_.erase( _name );
            return SUCCESS();
        }
        else {
            std::stringstream msg;
            msg << "resource has no child named [" << _name << "]";
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
        }

    } // remove_child

// =-=-=-=-=-=-=-
// public - mutator for parent pointer
    error resource::set_parent( const resource_ptr& _resc ) {
        parent_ = _resc;
        return SUCCESS();

    } // set_parent

// =-=-=-=-=-=-=-
// public - accessor for parent pointer.  return value based on validity of the pointer
    error resource::get_parent( resource_ptr& _resc ) {
        _resc = parent_;
        if ( _resc.get() ) {
            return SUCCESS();
        }
        else {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null parent pointer" );
        }

    } // get_parent

// =-=-=-=-=-=-=-
// public - set a name for the developer provided start op
    void resource::set_start_operation( const std::string& _op ) {
        start_opr_name_ = _op;
    } // resource::set_start_operation

// =-=-=-=-=-=-=-
// public - set a name for the developer provided stop op
    void resource::set_stop_operation( const std::string& _op ) {
        stop_opr_name_ = _op;
    } // resource::set_stop_operation

// END resource
// =-=-=-=-=-=-=-

// =-=-=-=-=-=-=-
// function to load and return an initialized resource plugin
    error load_resource_plugin( resource_ptr&     _plugin,
                                const std::string _plugin_name,
                                const std::string _inst_name,
                                const std::string _context ) {
        resource* resc = 0;
        error ret = load_plugin< resource >(
                        resc,
                        _plugin_name,
                        PLUGIN_TYPE_RESOURCE,
                        _inst_name,
                        _context );
        if ( ret.ok() && resc ) {
            _plugin.reset( resc );
        }
        else {
            if ( ret.code() == PLUGIN_ERROR_MISSING_SHARED_OBJECT ) {
                rodsLog(
                    LOG_DEBUG,
                    "loading impostor resource for [%s] of type [%s] with context [%s] and load_plugin message [%s]",
                    _inst_name.c_str(),
                    _plugin_name.c_str(),
                    _context.c_str(),
                    ret.result().c_str());
                _plugin.reset(
                    new impostor_resource(
                        "impostor_resource", "" ) );
            } else {
                return PASS( ret );
            }
        }

        return SUCCESS();

    } // load_resource_plugin

}; // namespace irods
