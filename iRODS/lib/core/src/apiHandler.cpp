// =-=-=-=-=-=-=-
// irods includes
#include "apiHandler.hpp"
#include "irods_load_plugin.hpp"
#include "irods_plugin_name_generator.hpp"
#include "irods_pack_table.hpp"
#include "irods_client_api_table.hpp"

#include <boost/filesystem.hpp>
namespace irods {

    void clearInStruct_noop( void* ) {};

// =-=-=-=-=-=-=-
// public - ctor
    api_entry::api_entry(
        apidef_t& _def ) :
        plugin_base( "this", "that" ),
        apiNumber( _def.apiNumber ),
        apiVersion( _def.apiVersion ),
        clientUserAuth( _def.clientUserAuth ),
        proxyUserAuth( _def.proxyUserAuth ),
        inPackInstruct( _def.inPackInstruct ),
        inBsFlag( _def.inBsFlag ),
        outPackInstruct( _def.outPackInstruct ),
        outBsFlag( _def.outBsFlag ),
        svrHandler( _def.svrHandler ),
        clearInStruct( _def.clearInStruct ) {
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
        svrHandler( _rhs.svrHandler ),
        clearInStruct( _rhs.clearInStruct ) {
    } // cctor

// =-=-=-=-=-=-=-
// public - assignment operator
    api_entry& api_entry::operator=(
        const api_entry& _rhs ) {
        if ( this == &_rhs ) {
            return *this;
        }
        apiNumber       = _rhs.apiNumber;
        apiVersion      = _rhs.apiVersion;
        clientUserAuth  = _rhs.clientUserAuth;
        proxyUserAuth   = _rhs.proxyUserAuth;
        inPackInstruct  = _rhs.inPackInstruct;
        inBsFlag        = _rhs.inBsFlag;
        outPackInstruct = _rhs.outPackInstruct;
        outBsFlag       = _rhs.outBsFlag;
        svrHandler      = _rhs.svrHandler;
        return *this;
    } // assignment op


// =-=-=-=-=-=-=-
// public - load operations from the plugin
    error api_entry::delay_load( void* _h ) {
        if ( !fcn_name_.empty() ) {
            svrHandler = ( funcPtr )dlsym( _h, fcn_name_.c_str() );
            if ( !svrHandler ) {
                std::string msg( "dlerror was empty" );
                char* err = dlerror();
                if ( err ) {
                    msg = err;
                }

                return ERROR( PLUGIN_ERROR, msg.c_str() );
            }
        }

        return SUCCESS();
    }

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
// public - cctor for pack entry
    pack_entry::pack_entry( const pack_entry& _rhs ) {
        packInstruct = _rhs.packInstruct;
        //clearInStruct = _rhs.clearInStruct;
    }

// =-=-=-=-=-=-=-
// public - assignment operator for pack entry
    pack_entry& pack_entry::operator=( const pack_entry& _rhs ) {
        packInstruct = _rhs.packInstruct;
        //clearInStruct = _rhs.clearInStruct;

        return *this;
    }

// =-=-=-=-=-=-=-
// public - ctor for api entry table
    pack_entry_table::pack_entry_table(
        packInstructArray_t _defs[] ) {
        // =-=-=-=-=-=-=-
        // initialize table from input array
        int i = 0;
        std::string end_str( PACK_TABLE_END_PI );
        while ( end_str != _defs[ i ].name ) {
            table_[ _defs[ i ].name ].packInstruct = _defs[i].packInstruct;
            ++i;
        } // for i

    } // ctor

// =-=-=-=-=-=-=-
// public - dtor for api entry table
    pack_entry_table::~pack_entry_table() {
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
        if( !ret.ok() ) {
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
                                plugin_home,
                                "inst", "ctx" );
                if ( ret.ok() && entry ) {
                    // =-=-=-=-=-=-=-
                    //
                    rodsLog(
                        LOG_DEBUG,
                        "init_api_table :: adding %d - [%s]",
                        entry->apiNumber,
                        entry->fcn_name_.c_str() );

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

#ifdef __cplusplus
extern "C" {
#endif
void init_client_api_table() {
    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );
}
#ifdef __cplusplus
}
#endif

