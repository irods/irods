#ifndef IRODS_RESOURCE_MANAGER_HPP
#define IRODS_RESOURCE_MANAGER_HPP

#include "irods/rods.h"
#include "irods/irods_resource_plugin.hpp"
#include "irods/irods_first_class_object.hpp"

#include <fmt/format.h>

#include <functional>
#include <string>
#include <string_view>

namespace irods
{
    extern const std::string EMPTY_RESC_HOST;
    extern const std::string EMPTY_RESC_PATH;

    /// definition of the resource interface
    extern const std::string RESOURCE_INTERFACE;

    /// special resource for local file system operations only
    extern const std::string LOCAL_USE_ONLY_RESOURCE;
    extern const std::string LOCAL_USE_ONLY_RESOURCE_VAULT;
    extern const std::string LOCAL_USE_ONLY_RESOURCE_TYPE;

    class resource_manager // NOLINT(cppcoreguidelines-special-member-functions)
    {
    public:
        // clang-format off
        using leaf_bundle_t = std::vector<rodsLong_t>;
        using iterator      = lookup_table<resource_ptr>::iterator;
        // clang-format on

        resource_manager() = default;

        resource_manager(const resource_manager& _other) = default;
        resource_manager& operator=(const resource_manager& _other) = default;

        // TODO Should this be virtual?
        virtual ~resource_manager() = default;

        /// Resolves a resource from a key into the resource table.
        error resolve(const std::string& _resc_key, resource_ptr& _resc_ptr);

        // Resolve a resource from a key into the resource table
        error resolve(rodsLong_t _resc_id, resource_ptr& _resc_ptr);

        // Resolves a resource from a match with a given property
        error validate_vault_path(const std::string& _physical_path,
                                  const rodsServerHost* _host,
                                  std::string& _vault_path);

        /// Populate resource table from catalog.
        error init_from_catalog(RsComm& _comm);

        /// Initialize a specific resource from the catalog.
        ///
        /// \param[in] _comm
        /// \param[in] _resc_name
        ///
        /// \return An iRODS error object containing the status of the operation.
        error init_from_catalog(RsComm& _comm, const std::string_view _resc_name);

        /// call shutdown on resources before destruction
        error shut_down_resources();

        /// TODO
        error shut_down_resource(const std::string_view _resc_name);

        /// TODO
        error shut_down_resource(rodsLong_t _resc_id);

        /// load a resource plugin given a resource type
        error init_from_type(const std::string& _resc_type,
                             const std::string& _resc_name,
                             const std::string& _instance_name,
                             const std::string& _resc_context,
                             resource_ptr& _resc_ptr);

        /// create a list of resources who do not have parents ( roots )
        error get_root_resources(std::vector<std::string>& _resources);

        /// create a partial hier string for a given resource to the root
        error get_hier_to_root_for_resc(const std::string& _resc_name, std::string& _resc_hier);

        /// create a partial hier string for a given resource to the root
        ///
        /// \param[in] _resource_name
        ///
        /// \returns std::string
        /// \retval resource hierarchy from provided resource name to root resource
        ///
        /// \throws irods::exception
        ///
        /// \since 4.2.9
        std::string get_hier_to_root_for_resc(std::string_view _resc_name);

        /// groups decedent leafs by child
        std::vector<leaf_bundle_t> gather_leaf_bundles_for_resc(const std::string& _resc_name);

        /// print the list of local resources out to stderr
        void print_local_resources();

        /// determine if any pdmos need to run before doing a connection
        bool need_maintenance_operations();

        /// exec the pdmos ( post disconnect maintenance operations ) in order
        int call_maintenance_operations(RcComm* _comm);

        /// construct a vector of all resource hierarchies in the system
        std::vector<std::string> get_all_resc_hierarchies();

        /// get the resc id of the leaf resource in the hierarchy
        error hier_to_leaf_id( const std::string&, rodsLong_t& );

        /// get the resc id of the leaf resource in the hierarchy
        ///
        /// \param[in] _hierarchy
        ///
        /// \retval leaf resource ID for given resource hierarchy
        ///
        /// \throws irods::exception
        ///
        /// \since 4.2.9
        rodsLong_t hier_to_leaf_id(std::string_view _hierarchy);

        /// get the resc hier of the resource given an id
        error leaf_id_to_hier( const rodsLong_t& _resc_id, std::string& _resc_hier);

        /// get the resource hierarchy string from provided leaf resource ID
        ///
        /// \param[in] _leaf_resource_id
        ///
        /// \retval resource hierarchy for given leaf resource ID
        ///
        /// \throws irods::exception
        ///
        /// \since 4.2.9
        std::string leaf_id_to_hier(const rodsLong_t _leaf_resource_id);

        /// get the resc name of the resource given an id
        error resc_id_to_name(const rodsLong_t& _resc_id, std::string& _resc_name);

        /// get the resc name of the resource given an id as a string
        error resc_id_to_name(const std::string& _resc_id, std::string& _resc_name);

        /// get the resc name of the resource given an id
        ///
        /// \param[in] _id
        ///
        /// \retval resource name for given resource id
        ///
        /// \throws irods::exception
        ///
        /// \since 4.2.9
        std::string resc_id_to_name(const rodsLong_t& _id);

        /// get the resc name of the resource given an id as a string
        ///
        /// \param[in] _id
        ///
        /// \retval resource name for given resource id
        ///
        /// \throws irods::exception
        ///
        /// \since 4.2.9
        std::string resc_id_to_name(std::string_view _id);

        /// check whether the specified resource name is a coordinating resource
        error is_coordinating_resource(const std::string& _resc_name, bool& _result);

        /// check whether the specified resource name is a coordinating resource
        bool is_coordinating_resource(const std::string& _resc_name);

        /// resolve a resource from a match with a given property key
        template <typename ValueType>
        error resolve_from_property(const std::string& _key, const ValueType& _value, resource_ptr& _resc_ptr)
        {
            // quick check on the resource table
            if (resource_name_map_.empty()) {
                return ERROR(SYS_INVALID_INPUT_PARAM, "empty resource table");
            }

            // simple flag to state a resource matching the prop and value is found
            bool found = false;

            // iterate through the map and search for our path
            for (auto&& [name, resc_ptr] : resource_name_map_) {
                // query resource for the property value
                ValueType value;
                error ret = resc_ptr->get_property<ValueType>(_key, value);

                // if we get a good parameter
                if (ret.ok()) {
                    // compare incoming value and stored value, assumes that the
                    // values support the comparison operator
                    if (_value == value) {
                        // if we get a match, cache the resource pointer
                        // in the given out variable and bail
                        found = true;
                        _resc_ptr = resc_ptr;
                        break;
                    }
                }
            }

            // did we find a resource and is the ptr valid?
            if (found) {
                return SUCCESS();
            }

            constexpr char* msg = "failed to find resource for property [{}] and value";
            return ERROR(SYS_RESC_DOES_NOT_EXIST, fmt::format(msg, _key));
        } // resolve_from_property

        /// TODO
        void add_child_to_resource(rodsLong_t _parent_resc_id, rodsLong_t _child_resc_id);

        /// TODO
        void add_child_to_resource(const std::string_view _parent_resc_name, const std::string_view _child_resc_name);

        /// TODO
        void remove_child_from_resource(rodsLong_t _parent_resc_id, rodsLong_t _child_resc_id);

        /// TODO
        void remove_child_from_resource(const std::string_view _parent_resc_name, const std::string_view _child_resc_name);

        /// TODO
        void erase_resource(rodsLong_t _resc_id);

        /// TODO
        void erase_resource(const std::string_view _resc_name);

        iterator begin()
        {
            return resource_name_map_.begin();
        }

        iterator end()
        {
            return resource_name_map_.end();
        }

    private:
        /// TODO
        void init_resource_from_database_row(const std::vector<std::string>& _row);

        /// Initialize the child map from the resources lookup table
        error init_parent_child_relationships();

        /// TODO
        error init_parent_child_relationship(const std::string_view _resc_name);

        void init_parent_child_relationship_impl(const std::string_view _resc_name, resource_ptr _resc_ptr);

        /// top level function to gather the post disconnect maintenance 
        /// operations from the resources, in breadth first order
        error gather_operations();

        /// TODO
        error gather_operations(const std::string_view _resc_name);

        /// top level function to call the start operation on the resource plugins
        error start_resource_plugins();

        /// TODO
        error start_resource_plugin(const std::string_view _resc_name);

        /// lower level recursive call to gather the post disconnect
        /// maintenance operations from the resources, in breadth first order
        error gather_operations_recursive(const std::string& _children,
                                          std::vector<std::string>& _processed_resources,
                                          std::vector<pdmo_type>& _gathered_ops);

        /// helper function for gather_leaf_bundles_for_resc
        error gather_leaf_bundle_for_child(const std::string&, leaf_bundle_t&);

        /// given a resource name get the parent name from the id
        error get_parent_name(resource_ptr _resc_ptr, std::string& _parent_name);

        lookup_table<resource_ptr> resource_name_map_;
        lookup_table<resource_ptr, long, std::hash<long>> resource_id_map_;
        std::vector<std::vector<pdmo_type>> maintenance_operations_;
    }; // class resource_manager
} // namespace irods

#endif // IRODS_RESOURCE_MANAGER_HPP

