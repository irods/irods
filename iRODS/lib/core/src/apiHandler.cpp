


// =-=-=-=-=-=-=-
// irods includes
#include "apiHandler.hpp"
#include "irods_api_home.hpp"
#include "irods_load_plugin.hpp"
#include "irods_plugin_name_generator.hpp"

#include <boost/filesystem.hpp>
namespace irods {

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
        svrHandler( _def.svrHandler ) {
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
        svrHandler( _rhs.svrHandler ) {
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

        svrHandler = ( funcPtr )dlsym( _h, fcn_name_.c_str() );
        char* err = 0;
        if ( !svrHandler || ( ( err = dlerror() ) != 0 ) ) {
            return ERROR( PLUGIN_ERROR, err );
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
    // public - ctor for api entry table
    pack_entry_table::pack_entry_table(
        packInstructArray_t _defs[] ) {
        // =-=-=-=-=-=-=-
        // initialize table from input array
        int i = 0;
        std::string end_str( PACK_TABLE_END_PI );
        while ( end_str != _defs[ i ].name ) {
            table_[ _defs[ i ].name ] = _defs[ i ].packInstruct;
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
        pack_entry_table& _pack_tbl ) {
        // =-=-=-=-=-=-=-
        // iterate over the API_HOME directory entries
        boost::filesystem::path so_dir( irods::API_HOME );
        if ( boost::filesystem::exists( so_dir ) ) {
            for ( boost::filesystem::directory_iterator it( so_dir );
                    it != boost::filesystem::directory_iterator();
                    ++it ) {
                // =-=-=-=-=-=-=-
                // given a shared object, load the plugin from it
                std::string name  = it->path().stem().string();

                size_t pos = name.find( "lib" );
                if ( std::string::npos == pos ) {
                    continue;
                }

                name = name.substr( 3 );
#if 1
                api_entry*  entry = 0;
                error ret = load_plugin< api_entry >(
                                entry,
                                name,
                                API_HOME,
                                "inst", "ctx" );
                if ( ret.ok() && entry ) {
                    // =-=-=-=-=-=-=-
                    // ask the plugin to fill in the api and pack
                    // tables with its appropriate values
                    _api_tbl[ entry->apiNumber ].reset( entry );

                    // =-=-=-=-=-=-=-
                    // add the in struct
                    if ( strlen( entry->in_pack_key ) > 0 ) {
                        _pack_tbl[ entry->in_pack_key ] = entry->inPackInstruct;
                    }

                    // =-=-=-=-=-=-=-
                    // add the out struct
                    if ( strlen( entry->out_pack_key ) > 0 ) {
                        _pack_tbl[ entry->out_pack_key ] = entry->outPackInstruct;
                    }
                }
                else {
                    irods::log( PASS( ret ) );

                }
#endif
            } // for itr
        } // if exists

        return SUCCESS();

    } // init_api_table

}; // namespace irods



