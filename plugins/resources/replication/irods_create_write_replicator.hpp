#ifndef _IRODS_CREATE_WRITE_REPLICATOR_HPP_
#define _IRODS_CREATE_WRITE_REPLICATOR_HPP_

#include "irods_error.hpp"
#include "irods_oper_replicator.hpp"

namespace irods {

    /**
     * @brief Replicator for create/write operations
     */
    class create_write_replicator : public oper_replicator {
        public:
            /// @brief ctor. Note the resource is the root resource of the whole tree
            create_write_replicator(
                const std::string& _root_resource,    // The name of the resource at the root of the hierarchy
                const std::string& _current_resource, // The name of the resource at this level of hierarchy
                const std::string& _child );          // The hierarchy of the child.
            virtual ~create_write_replicator( void );

            error replicate( plugin_context& _ctx, const child_list_t& _siblings, const object_oper& _object_oper );

        private:
            std::string root_resource_;
            std::string current_resource_;
            std::string child_;
    };
}; // namespace irods

#endif // _IRODS_CREATE_WRITE_REPLICATOR_HPP_
