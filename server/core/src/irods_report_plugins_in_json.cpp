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

        for (auto & itr : plugin_list) {

            json_t* plug = json_object();
            json_object_set_new( plug, "name",     json_string( itr.c_str() ) );
            json_object_set_new( plug, "type",     json_string( _type_name ) );
            json_object_set_new( plug, "version",  json_string( "" ) );
            json_object_set_new( plug, "checksum_sha256", json_string( "" ) );

            json_array_append_new( _json_array, plug );
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

    error serialize_resource_plugin_to_json(
        const resource_ptr& _resc,
        json_t*             _entry ) {
        if ( !_entry ) {
            return ERROR(
                       SYS_NULL_INPUT,
                       "null json object _entry" );
        }

        std::string host_name;
        error ret = _resc->get_property< std::string >( irods::RESOURCE_LOCATION, host_name );
        if ( !ret.ok() ) {
            return PASS(ret);
        }

        std::string name;
        ret = _resc->get_property< std::string >( irods::RESOURCE_NAME, name );
        if ( !ret.ok() ) {
            return PASS(ret);
        }

        std::string type;
        ret = _resc->get_property< std::string >( irods::RESOURCE_TYPE, type );
        if ( !ret.ok() ) {
            return PASS(ret);
        }

        std::string vault;
        ret = _resc->get_property< std::string >( irods::RESOURCE_PATH, vault );
        if ( !ret.ok() ) {
            return PASS(ret);
        }

        std::string context;
        ret = _resc->get_property< std::string >( irods::RESOURCE_CONTEXT, context );
        if ( !ret.ok() ) {
            return PASS(ret);
        }

        std::string parent;
        ret = _resc->get_property< std::string >( irods::RESOURCE_PARENT, parent );
        if ( !ret.ok() ) {
            return PASS(ret);
        }

        std::string parent_context;
        ret = _resc->get_property< std::string >( irods::RESOURCE_PARENT_CONTEXT, parent_context );
        if ( !ret.ok() ) {
            return PASS(ret);
        }

        std::string freespace;
        ret = _resc->get_property< std::string >( irods::RESOURCE_FREESPACE, freespace );
        if ( !ret.ok() ) {
            return PASS(ret);
        }

        int status = 0;
        ret = _resc->get_property< int >( irods::RESOURCE_STATUS, status );
        if ( !ret.ok() ) {
            return PASS(ret);
        }

        json_object_set_new( _entry, "name",            json_string( name.c_str() ) );
        json_object_set_new( _entry, "type",            json_string( type.c_str() ) );
        json_object_set_new( _entry, "host",            json_string( host_name.c_str() ) );
        json_object_set_new( _entry, "vault_path",      json_string( vault.c_str() ) );
        json_object_set_new( _entry, "context_string",  json_string( context.c_str() ) );
        json_object_set_new( _entry, "parent_resource", json_string( parent.c_str() ) );
        json_object_set_new( _entry, "parent_context",  json_string( parent_context.c_str() ) );
        json_object_set_new( _entry, "free_space",      json_string( freespace.c_str() ) );

        if ( status != INT_RESC_STATUS_DOWN ) {
            json_object_set_new( _entry, "status", json_string( "up" ) );
        }
        else {
            json_object_set_new( _entry, "status", json_string( "down" ) );
        }

        return SUCCESS();
    }

}; // namespace irods
