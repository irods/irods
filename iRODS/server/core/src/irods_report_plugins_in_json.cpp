#include "irods_report_plugins_in_json.hpp"
#include "irods_configuration_keywords.hpp"
#include "boost/lexical_cast.hpp"


namespace irods {
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

        std::string children;
        ret = _resc->get_property< std::string >( irods::RESOURCE_CHILDREN, children );
        if ( !ret.ok() ) {
            return PASS(ret);
        }

        std::string object_count;
        ret = _resc->get_property< std::string >( irods::RESOURCE_OBJCOUNT, object_count );
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

        json_object_set( _entry, "name",            json_string( name.c_str() ) );
        json_object_set( _entry, "type",            json_string( type.c_str() ) );
        json_object_set( _entry, "host",            json_string( host_name.c_str() ) );
        json_object_set( _entry, "vault_path",      json_string( vault.c_str() ) );
        json_object_set( _entry, "context_string",  json_string( context.c_str() ) );
        json_object_set( _entry, "parent_resource", json_string( parent.c_str() ) );
        json_object_set( _entry, "children",        json_string( children.c_str() ) );
        json_object_set( _entry, "free_space",      json_string( freespace.c_str() ) );

        try {
            json_int_t object_count_int = boost::lexical_cast<json_int_t>(object_count);
            json_object_set( _entry, "object_count", json_integer( object_count_int ) );
        } catch ( const boost::bad_lexical_cast& ) {
            json_object_set( _entry, "object_count", json_integer( -1 ) );
        }

        if ( status != INT_RESC_STATUS_DOWN ) {
            json_object_set( _entry, "status", json_string( "up" ) );
        }
        else {
            json_object_set( _entry, "status", json_string( "down" ) );
        }

        return SUCCESS();
    }

}; // namespace irods
