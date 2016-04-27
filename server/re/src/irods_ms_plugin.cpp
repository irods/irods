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
        num_args_( 0 ) {
    } // def ctor

    ms_table_entry::ms_table_entry(
        int    _n ) :
        plugin_base(
            "msvc",
            "ctx" ),
        num_args_( _n ) {
    } // ctor

    ms_table_entry::ms_table_entry(
        const std::string& _name,
        unsigned int                _num_args,
        boost::any         _fcn_ptr ) :
        plugin_base(
            "msvc",
            "ctx" ),
        operation_name_( _name ),
        num_args_( _num_args ) {
        operations_[operation_name_] = _fcn_ptr;
    } // ctor

    ms_table_entry::ms_table_entry(
        const ms_table_entry& _rhs ) :
        plugin_base( _rhs ),
        operation_name_( _rhs.operation_name_ ),
        num_args_( _rhs.num_args_ ) {
    } // cctor

    ms_table_entry& ms_table_entry::operator=(
        const ms_table_entry& _rhs ) {
        plugin_base::operator=( _rhs );
        num_args_       = _rhs.num_args_;
        operation_name_ = _rhs.operation_name_;
        return *this;
    } // operator=

    ms_table_entry::~ms_table_entry() {
    } // dtor

    int ms_table_entry::call(
        ruleExecInfo_t*          _rei,
        std::vector<msParam_t*>& _params ) {
        if( _params.size() != num_args_ ) {
            return SYS_INVALID_INPUT_PARAM;
        }

        int status = 0;
        if ( _params.size() == 0 ) {
            status = call_handler<ruleExecInfo_t*>( _rei );
        }
        else if ( _params.size() == 1 ) {
            status = call_handler<
                msParam_t*,
                ruleExecInfo_t*>(
                        _params[0],
                        _rei );
        }
        else if ( _params.size() == 2 ) {
            status = call_handler<
                msParam_t*,
                msParam_t*,
                ruleExecInfo_t*>(
                        _params[0],
                        _params[1],
                        _rei );
        }
        else if ( _params.size() == 3 ) {
            status = call_handler<
                msParam_t*,
                msParam_t*,
                msParam_t*,
                ruleExecInfo_t*>(
                        _params[0],
                        _params[1],
                        _params[2],
                        _rei );
        }
        else if ( _params.size() == 4 ) {
            status = call_handler<
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                ruleExecInfo_t*>(
                        _params[0],
                        _params[1],
                        _params[2],
                        _params[3],
                        _rei );

        }
        else if ( _params.size() == 5 ) {
            status = call_handler<
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                ruleExecInfo_t*>(
                        _params[0],
                        _params[1],
                        _params[2],
                        _params[3],
                        _params[4],
                        _rei );
        }
        else if ( _params.size() == 6 ) {
            status = call_handler<
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                ruleExecInfo_t*>(
                        _params[0],
                        _params[1],
                        _params[2],
                        _params[3],
                        _params[4],
                        _params[5],
                        _rei );
        }
        else if ( _params.size() == 7 ) {
            status = call_handler<
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                ruleExecInfo_t*>(
                        _params[0],
                        _params[1],
                        _params[2],
                        _params[3],
                        _params[4],
                        _params[5],
                        _params[6],
                        _rei );

        }
        else if ( _params.size() == 8 ) {
            status = call_handler<
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                ruleExecInfo_t*>(
                        _params[0],
                        _params[1],
                        _params[2],
                        _params[3],
                        _params[4],
                        _params[5],
                        _params[6],
                        _params[7],
                        _rei );
        }
        else if ( _params.size() == 9 ) {
            status = call_handler<
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                ruleExecInfo_t*>(
                        _params[0],
                        _params[1],
                        _params[2],
                        _params[3],
                        _params[4],
                        _params[5],
                        _params[6],
                        _params[7],
                        _params[8],
                        _rei );
        }
        else if ( _params.size() == 10 ) {
            status = call_handler<
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                msParam_t*,
                ruleExecInfo_t*>(
                        _params[0],
                        _params[1],
                        _params[2],
                        _params[3],
                        _params[4],
                        _params[5],
                        _params[6],
                        _params[7],
                        _params[8],
                        _params[9],
                        _rei );
        }

        return status;

    } // call


// =-=-=-=-=-=-=-
// given the name of a microservice, try to load the shared object
// and then register that ms with the table
    error load_microservice_plugin( ms_table& _table, const std::string _ms ) {
        ms_table_entry* entry = 0;
        error load_err = load_plugin< ms_table_entry >(
                             entry,
                             _ms,
                             PLUGIN_TYPE_MICROSERVICE,
                             "msvc", "ctx" );
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
