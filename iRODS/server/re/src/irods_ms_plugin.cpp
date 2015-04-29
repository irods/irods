// =-=-=-=-=-=-=-
// My Includes
#include "irods_ms_plugin.hpp"
#include "irods_load_plugin.hpp"
#include "irods_log.hpp"

// =-=-=-=-=-=-=-
// STL Includes
#include <iostream>
#include <sstream>

namespace irods {
// =-=-=-=-=-=-=-
// ms_table_entry definition
    ms_table_entry::ms_table_entry( ) :
        plugin_base(
            "",
            "" ),
        num_args_( 0 ),
        call_action_( 0 ) {
    } // def ctor

    ms_table_entry::ms_table_entry(
        int    _n ) :
        plugin_base(
            "msvc",
            "ctx" ),
        num_args_( _n ),
        call_action_( 0 ) {
    } // ctor

    ms_table_entry::ms_table_entry(
        const std::string&, //_name
        int                _num_args,
        ms_func_ptr        _fcn_ptr ) :
        plugin_base(
            "msvc",
            "ctx" ),
        num_args_( _num_args ),
        call_action_( _fcn_ptr ) {
    } // ctor

    ms_table_entry::ms_table_entry(
        const ms_table_entry& _rhs ) :
        plugin_base( _rhs ),
        num_args_( _rhs.num_args_ ),
        call_action_( _rhs.call_action_ ) {
    } // cctor

    ms_table_entry& ms_table_entry::operator=(
        const ms_table_entry& _rhs ) {
        plugin_base::operator=( _rhs );
        num_args_    = _rhs.num_args_;
        call_action_ = _rhs.call_action_;
        return *this;
    } // operator=

    ms_table_entry::~ms_table_entry() {
    } // dtor

    error ms_table_entry::delay_load(
        void* _h ) {
        // =-=-=-=-=-=-=-
        // check handle to open shared object
        if ( !_h ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null handle parameter" );
        }

        // =-=-=-=-=-=-=-
        // check to see if we actually have any ops
        if ( 0 == ops_for_delay_load_.size() ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "no ops to load" );
        }

        // =-=-=-=-=-=-=-
        // load our operation, assuming we have only one
        std::string action = ops_for_delay_load_[0].first;
        call_action_ = reinterpret_cast< ms_func_ptr >( dlsym( _h, action.c_str() ) );


        // =-=-=-=-=-=-=-
        // check our fcn ptr
        if ( !call_action_ ) {
            std::stringstream msg;
            msg << "failed to load msvc function [";
            msg << action;
            msg << "]";
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
        }

        return SUCCESS();

    } // delay_load

// =-=-=-=-=-=-=-
// given the name of a microservice, try to load the shared object
// and then register that ms with the table
    error load_microservice_plugin( ms_table& _table, const std::string _ms ) {
        // =-=-=-=-=-=-=-
        // resolve plugin directory
        std::string plugin_home;
        error ret = resolve_plugin_path( PLUGIN_TYPE_MICROSERVICE, plugin_home );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        ms_table_entry* entry = 0;
        error load_err = load_plugin< ms_table_entry >( entry, _ms, plugin_home, "msvc", "ctx" );
        if ( load_err.ok() && entry ) {
            _table[ _ms ] = entry;
            return SUCCESS();
        }
        else {
            error ret = PASSMSG( "Failed to create ms plugin entry.", load_err );
            // JMC :: spams the log - log( ret );
            return ret;
        }
    } // load_microservice_plugin

}; // namespace irods
