


// =-=-=-=-=-=-=-
// irods includes
#include "apiHandler.hpp"

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
            table_[ _defs[ i ].apiNumber ] = new api_entry( _defs[ i ] );

        } // for i

    } // ctor

    // =-=-=-=-=-=-=-
    // public - dtor for api entry table
    api_entry_table::~api_entry_table() {
        // =-=-=-=-=-=-=-
        // clean up table
        api_entry_table::iterator itr = begin();
        for ( ; itr != end(); ++itr ) {
            delete itr->second;
        }
    } // dtor

}; // namespace irods



