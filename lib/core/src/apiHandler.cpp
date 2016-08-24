// =-=-=-=-=-=-=-
// irods includes
#include "apiHandler.hpp"
#include "irods_load_plugin.hpp"
#include "irods_plugin_name_generator.hpp"
#include "irods_pack_table.hpp"
#include "irods_client_api_table.hpp"
#include <boost/filesystem.hpp>
namespace irods {



// =-=-=-=-=-=-=-
// public - ctor
    api_entry::api_entry(
        apidef_t& _def ) :
        plugin_base( "api_instance", "api_context" ),
        apiNumber( _def.apiNumber ),
        apiVersion( _def.apiVersion ),
        clientUserAuth( _def.clientUserAuth ),
        proxyUserAuth( _def.proxyUserAuth ),
        inPackInstruct( _def.inPackInstruct ),
        inBsFlag( _def.inBsFlag ),
        outPackInstruct( _def.outPackInstruct ),
        outBsFlag( _def.outBsFlag ),
        call_wrapper(_def.call_wrapper),
        operation_name(_def.operation_name),
        clearInStruct( _def.clearInStruct ) {
        operations_[ _def.operation_name ] = _def.svrHandler;
    } // ctor

// =-=-=-=-=-=-=-
// public - copy ctor
    api_entry::api_entry(
        const api_entry& _rhs ) :
        plugin_base( _rhs ),
        apiNumber( _rhs.apiNumber ),
        apiVersion( _rhs.apiVersion ),
        clientUserAuth( _rhs.clientUserAuth ),
        proxyUserAuth( _rhs.proxyUserAuth ),
        inPackInstruct( _rhs.inPackInstruct ),
        inBsFlag( _rhs.inBsFlag ),
        outPackInstruct( _rhs.outPackInstruct ),
        outBsFlag( _rhs.outBsFlag ),
        call_wrapper(_rhs.call_wrapper),
        in_pack_key(_rhs.in_pack_key),
        out_pack_key(_rhs.out_pack_key),
        in_pack_value(_rhs.in_pack_value),
        out_pack_value(_rhs.out_pack_value),
        operation_name(_rhs.operation_name),
        extra_pack_struct(_rhs.extra_pack_struct),
        clearInStruct( _rhs.clearInStruct ) {
    } // cctor

// =-=-=-=-=-=-=-
// public - assignment operator
    api_entry& api_entry::operator=(
        const api_entry& _rhs ) {
        if ( this == &_rhs ) {
            return *this;
        }
        plugin_base::operator=(_rhs);
        apiNumber       = _rhs.apiNumber;
        apiVersion      = _rhs.apiVersion;
        clientUserAuth  = _rhs.clientUserAuth;
        proxyUserAuth   = _rhs.proxyUserAuth;
        inPackInstruct  = _rhs.inPackInstruct;
        inBsFlag        = _rhs.inBsFlag;
        outPackInstruct = _rhs.outPackInstruct;
        outBsFlag       = _rhs.outBsFlag;
        return *this;
    } // assignment op

// =-=-=-=-=-=-=-
// public - ctor for api entry table
    api_entry_table::api_entry_table(
        apidef_t _defs[],
        size_t   _num ) {
        // =-=-=-=-=-=-=-
        // initialize table from input array
        for ( size_t i = 0; i < _num; ++i ) {
            table_[ _defs[ i ].apiNumber ] = api_entry_ptr( new api_entry( _defs[ i ] ) );

        } // for i

    } // ctor

// =-=-=-=-=-=-=-
// public - dtor for api entry table
    api_entry_table::~api_entry_table() {
    } // dtor






// =-=-=-=-=-=-=-
// public - load api plugins
    error init_api_table(
        api_entry_table&  _api_tbl,
        pack_entry_table& _pack_tbl,
        bool              _cli_flg ) {
        // =-=-=-=-=-=-=-
        // resolve plugin directory
        std::string plugin_home;
        error ret = resolve_plugin_path( irods::PLUGIN_TYPE_API, plugin_home );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // iterate over the API_HOME directory entries
        boost::filesystem::path so_dir( plugin_home );
        if ( boost::filesystem::exists( so_dir ) ) {
            for ( boost::filesystem::directory_iterator it( so_dir );
                    it != boost::filesystem::directory_iterator();
                    ++it ) {


                // =-=-=-=-=-=-=-
                // given a shared object, load the plugin from it
                std::string name  = it->path().stem().string();
                // =-=-=-=-=-=-=-
                // if client side, skip server plugins, etc.
                if ( _cli_flg ) {
                    size_t pos = name.find( "_server" );
                    if ( std::string::npos != pos ) {
                        continue;
                    }

                }
                else {
                    size_t pos = name.find( "_client" );
                    if ( std::string::npos != pos ) {
                        continue;
                    }

                }

                // =-=-=-=-=-=-=-
                // clip off the lib to remain compliant with
                // load_plugin's expected behavior
                size_t pos = name.find( "lib" );
                if ( std::string::npos == pos ) {
                    continue;
                }
                name = name.substr( 3 );

                api_entry*  entry = 0;
                error ret = load_plugin< api_entry >(
                                entry,
                                name,
                                PLUGIN_TYPE_API,
                                "api_instance",
                                "api_context" );
                if ( ret.ok() && entry ) {
                    // =-=-=-=-=-=-=-
                    //
                    rodsLog(
                        LOG_DEBUG,
                        "init_api_table :: adding %d - [%s] - [%s]",
                        entry->apiNumber,
                        entry->operation_name.c_str(),
                        name.c_str() );

                    // =-=-=-=-=-=-=-
                    // ask the plugin to fill in the api and pack
                    // tables with its appropriate values
                    _api_tbl[ entry->apiNumber ] = api_entry_ptr( entry );

                    // =-=-=-=-=-=-=-
                    // add the in struct
                    if ( !entry->in_pack_key.empty() ) {
                        _pack_tbl[ entry->in_pack_key ].packInstruct = entry->in_pack_value;
                        entry->inPackInstruct = entry->in_pack_key.c_str();
                    }

                    // =-=-=-=-=-=-=-
                    // add the out struct
                    if ( !entry->out_pack_key.empty() ) {
                        _pack_tbl[ entry->out_pack_key ].packInstruct = entry->out_pack_value;
                        entry->outPackInstruct = entry->out_pack_key.c_str();
                    }

                    // =-=-=-=-=-=-=-
                    // some plugins may define additional packinstructions
                    // which are composites of the in or out structs
                    if ( !entry->extra_pack_struct.empty() ) {
                        lookup_table<std::string>::iterator itr;
                        for ( itr  = entry->extra_pack_struct.begin();
                                itr != entry->extra_pack_struct.end();
                                ++itr ) {
                            _pack_tbl[ itr->first ].packInstruct = itr->second;

                        } // for itr

                    } // if empty

                }
                else {
                    irods::log( PASS( ret ) );
                }

            } // for itr

        } // if exists

        return SUCCESS();

    } // init_api_table

}; // namespace irods
