
#include "irods_report_plugins_in_json.hpp"

namespace irods {
    error add_plugin_type_to_json_array(
        const std::string _plugin_type,
        const char*        _type_name,
        json_t*&           _json_array ) {

        std::string plugin_home;
        error ret = resolve_plugin_path( _plugin_type, plugin_home );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        plugin_name_generator name_gen;
        plugin_name_generator::plugin_list_t plugin_list;
        ret = name_gen.list_plugins( plugin_home, plugin_list );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        for ( plugin_name_generator::plugin_list_t::iterator itr = plugin_list.begin();
                itr != plugin_list.end();
                ++itr ) {

            json_t* plug = json_object();
            json_object_set( plug, "name",     json_string( itr->c_str() ) );
            json_object_set( plug, "type",     json_string( _type_name ) );
            json_object_set( plug, "version",  json_string( "" ) );
            json_object_set( plug, "checksum_sha256", json_string( "" ) );

            json_array_append( _json_array, plug );
        }

        return SUCCESS();
    }

    error get_plugin_array(
        json_t*& _plugins ) {
        _plugins = json_array();
        if ( !_plugins ) {
            return ERROR(
                       SYS_MALLOC_ERR,
                       "json_object() failed" );
        }

        error ret = add_plugin_type_to_json_array( PLUGIN_TYPE_RESOURCE, "resource", _plugins );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        std::string svc_role;
        ret = get_catalog_service_role(svc_role);
        if(!ret.ok()) {
            log(PASS(ret));
            return PASS( ret );
        }

        if( CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            ret = add_plugin_type_to_json_array( PLUGIN_TYPE_DATABASE, "database", _plugins );
            if ( !ret.ok() ) {
                return PASS( ret );
            }
        }

        ret = add_plugin_type_to_json_array( PLUGIN_TYPE_AUTHENTICATION, "authentication", _plugins );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        ret = add_plugin_type_to_json_array( PLUGIN_TYPE_NETWORK, "network", _plugins );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        ret = add_plugin_type_to_json_array( PLUGIN_TYPE_API, "api", _plugins );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        ret = add_plugin_type_to_json_array( PLUGIN_TYPE_MICROSERVICE, "microservice", _plugins );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        return SUCCESS();

    } // get_plugin_array

}; // namespace irods
