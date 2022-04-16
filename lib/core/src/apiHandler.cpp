// =-=-=-=-=-=-=-
// irods includes
#include "irods/apiHandler.hpp"
#include "irods/irods_load_plugin.hpp"
#include "irods/irods_plugin_name_generator.hpp"
#include "irods/irods_pack_table.hpp"
#include "irods/irods_client_api_table.hpp"

#include <boost/filesystem.hpp>

#include <algorithm>

namespace irods
{
    // =-=-=-=-=-=-=-
    // public - ctor
    api_entry::api_entry( apidef_t& _def )
        : plugin_base( "api_instance", "api_context" )
        , apiNumber( _def.apiNumber )
        , apiVersion( _def.apiVersion )
        , clientUserAuth( _def.clientUserAuth )
        , proxyUserAuth( _def.proxyUserAuth )
        , inPackInstruct( _def.inPackInstruct )
        , inBsFlag( _def.inBsFlag )
        , outPackInstruct( _def.outPackInstruct )
        , outBsFlag( _def.outBsFlag )
        , call_wrapper(_def.call_wrapper)
        , operation_name(_def.operation_name)
        , clearInStruct( _def.clearInStruct )
    {
        operations_[ _def.operation_name ] = _def.svrHandler;
    } // ctor

    // =-=-=-=-=-=-=-
    // public - copy ctor
    api_entry::api_entry( const api_entry& _rhs )
        : plugin_base( _rhs )
        , apiNumber( _rhs.apiNumber )
        , apiVersion( _rhs.apiVersion )
        , clientUserAuth( _rhs.clientUserAuth )
        , proxyUserAuth( _rhs.proxyUserAuth )
        , inPackInstruct( _rhs.inPackInstruct )
        , inBsFlag( _rhs.inBsFlag )
        , outPackInstruct( _rhs.outPackInstruct )
        , outBsFlag( _rhs.outBsFlag )
        , call_wrapper(_rhs.call_wrapper)
        , in_pack_key(_rhs.in_pack_key)
        , out_pack_key(_rhs.out_pack_key)
        , in_pack_value(_rhs.in_pack_value)
        , out_pack_value(_rhs.out_pack_value)
        , operation_name(_rhs.operation_name)
        , extra_pack_struct(_rhs.extra_pack_struct)
        , clearInStruct( _rhs.clearInStruct )
    {
    } // cctor

    // =-=-=-=-=-=-=-
    // public - assignment operator
    api_entry& api_entry::operator=( const api_entry& _rhs )
    {
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
    api_entry_table::api_entry_table(apidef_t defs[], size_t num)
        : loaded_plugins_{}
    {
        for (size_t i = 0; i < num; ++i) {
            table_[defs[i].apiNumber] = api_entry_ptr(new api_entry(defs[i]));
        }
    } // ctor

    auto api_entry_table::is_plugin_loaded(std::string_view plugin_name) -> bool
    {
        const auto end = std::cend(loaded_plugins_);
        return std::find(std::cbegin(loaded_plugins_), end, plugin_name) != end;
    }

    auto api_entry_table::mark_plugin_as_loaded(std::string_view plugin_name) -> void
    {
        if (!is_plugin_loaded(plugin_name)) {
            loaded_plugins_.push_back(plugin_name.data());
        }
    }

    // =-=-=-=-=-=-=-
    // public - load api plugins
    error init_api_table(api_entry_table&  _api_tbl,
                         pack_entry_table& _pack_tbl,
                         bool              _cli_flg)
    {
        // =-=-=-=-=-=-=-
        // resolve plugin directory
        std::string plugin_home;
        error ret = resolve_plugin_path( irods::KW_CFG_PLUGIN_TYPE_API, plugin_home );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        namespace fs = boost::filesystem;

        // =-=-=-=-=-=-=-
        // iterate over the API_HOME directory entries
        if (const fs::path so_dir(plugin_home); fs::exists(so_dir)) {
            for (fs::directory_iterator it(so_dir); it != fs::directory_iterator(); ++it) {
                const auto path = it->path();

                // Skip the API plugin if it was loaded before.
                if (_api_tbl.is_plugin_loaded(path.c_str())) {
                    const auto* msg = "init_api_table :: API plugin [%s] has already been loaded. Skipping ...";
                    rodsLog(LOG_DEBUG, msg, path.stem().c_str());
                    continue;
                }

                // =-=-=-=-=-=-=-
                // given a shared object, load the plugin from it
                std::string name = path.stem().string();

                // =-=-=-=-=-=-=-
                // if client side, skip server plugins, etc.
                if (std::string::npos != name.find(_cli_flg ? "_server" : "_client")) {
                    continue;
                }

                // =-=-=-=-=-=-=-
                // clip off the lib to remain compliant with
                // load_plugin's expected behavior
                size_t pos = name.find( "lib" );
                if ( std::string::npos == pos ) {
                    continue;
                }
                name = name.substr( 3 );

                api_entry* entry = nullptr;
                error ret = load_plugin<api_entry>(
                                entry,
                                name,
                                KW_CFG_PLUGIN_TYPE_API,
                                "api_instance",
                                "api_context");
                if (ret.ok() && entry) {
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
                    if (!entry->extra_pack_struct.empty()) {
                        for (auto&& [key, value] : entry->extra_pack_struct) {
                            _pack_tbl[key].packInstruct = value;
                        }
                    }

                    // Remember that the current plugin has been loaded.
                    // This keeps the server from wasting resources loading the same
                    // plugin multiple times.
                    _api_tbl.mark_plugin_as_loaded(path.c_str());
                }
                else {
                    irods::log( PASS( ret ) );
                }
            } // for itr
        } // if exists

        return SUCCESS();
    } // init_api_table
} // namespace irods

