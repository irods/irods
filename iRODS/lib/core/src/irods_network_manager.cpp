// =-=-=-=-=-=-=-
#include "irods_network_manager.hpp"
#include "irods_load_plugin.hpp"

namespace irods {
// =-=-=-=-=-=-=-
// network manager singleton
    network_manager netwk_mgr;

// =-=-=-=-=-=-=-
// public - Constructor
    network_manager::network_manager() {

    } // ctor

// =-=-=-=-=-=-=-
// public - Copy Constructor
    network_manager::network_manager( const network_manager& _rhs ) {
        plugins_ = _rhs.plugins_;

    } // cctor

// =-=-=-=-=-=-=-
// public - Destructor
    network_manager::~network_manager( ) {

    } // dtor

// =-=-=-=-=-=-=-
// public - retrieve a network plugin given its key
    error network_manager::resolve(
        std::string  _key,
        network_ptr& _value ) {

        if ( _key.empty() ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "empty key" );
        }

        if ( plugins_.has_entry( _key ) ) {
            _value = plugins_[ _key ];
            return SUCCESS();

        }
        else {
            std::stringstream msg;
            msg << "no network plugin found for name ["
                << _key
                << "]";
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );

        }

    } // resolve

    static error load_network_plugin(
        network_ptr&       _plugin,
        const std::string& _plugin_name,
        const std::string& _inst_name,
        const std::string& _context ) {
        // =-=-=-=-=-=-=-
        // call generic plugin loader
        network* net = 0;
        error ret = load_plugin< network >(
                        net,
                        _plugin_name,
                        PLUGIN_TYPE_NETWORK,
                        _inst_name,
                        _context );
        if ( ret.ok() && net ) {
            _plugin.reset( net );
            return SUCCESS();

        }
        else {
            return PASS( ret );

        }

    } // load_network_plugin

// =-=-=-=-=-=-=-
// public - given a type, load up a network plugin
    error network_manager::init_from_type(
        const int&         _proc_type,
        const std::string& _type,
        const std::string& _key,
        const std::string& _inst,
        const std::string& _ctx,
        network_ptr&       _net ) {

        std::string type = _type;
        if( CLIENT_PT == _proc_type ) {
            type += "_client";
        } 
        else {
            type += "_server";
        }

        // =-=-=-=-=-=-=-
        // create the network plugin and add it to the table
        network_ptr ptr;
        error ret = load_network_plugin( ptr, type, _inst, _ctx );
        if ( !ret.ok() ) {
            return PASSMSG( "Failed to load network plugin", ret );
        }

        plugins_[ _key ] = ptr;

        _net = plugins_[ _key ];

        return SUCCESS();

    } // init_from_type

}; // namespace irods



