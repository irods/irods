#ifndef IRODS_RESOURCE_PLUGIN_IMPOSTOR_HPP
#define IRODS_RESOURCE_PLUGIN_IMPOSTOR_HPP

#include "irods_resource_plugin.hpp"

namespace irods {

    class impostor_resource : public irods::resource {
        public:
            impostor_resource(
                const std::string& _inst_name,
                const std::string& _context );
            error need_post_disconnect_maintenance_operation( bool& _b );
            error post_disconnect_maintenance_operation( pdmo_type& _op );
            static error report_error(
                plugin_context& );

    }; // class impostor_resource

}; // namespace irods


#endif // IRODS_RESOURCE_PLUGIN_IMPOSTOR_HPP



