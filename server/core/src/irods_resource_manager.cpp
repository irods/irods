#include "irods/irods_resource_manager.hpp"

#include "irods/genQuery.h"
#include "irods/generalAdmin.h"
#include "irods/getRescQuota.h"
#include "irods/irods_children_parser.hpp"
#include "irods/irods_error.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/irods_lexical_cast.hpp"
#include "irods/irods_load_plugin.hpp"
#include "irods/irods_log.hpp"
#include "irods/irods_resource_plugin_impostor.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/irods_string_tokenize.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/rcMisc.h"
#include "irods/rodsErrorTable.h"
#include "irods/rsGenQuery.hpp"
#include "irods/rsGlobalExtern.hpp"
#include "irods/irods_logger.hpp"

#define IRODS_QUERY_ENABLE_SERVER_SIDE_API
#include "irods/query_builder.hpp"

#include <iostream>
#include <vector>
#include <iterator>

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables, cert-err58-cpp)
irods::resource_manager resc_mgr;

namespace
{
    using log_resc = irods::experimental::log::resource;

    irods::error load_resource_plugin(
        irods::resource_ptr& _plugin,
        const std::string& _plugin_name,
        const std::string& _instance_name,
        const std::string& _context)
    {
        irods::resource* resc_ptr{};
        const auto err = load_plugin<irods::resource>(
            resc_ptr, _plugin_name, irods::KW_CFG_PLUGIN_TYPE_RESOURCE, _instance_name, _context);

        if (!err.ok()) {
            if (err.code() != PLUGIN_ERROR_MISSING_SHARED_OBJECT) {
                return PASS(err);
            }

            log_resc::debug(
                "loading impostor resource for [{}] of type [{}] with context [{}] and load_plugin message [{}]",
                _instance_name, _plugin_name, _context, err.result());

            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
            _plugin.reset(new irods::impostor_resource("impostor_resource", ""));
        }

        _plugin.reset(resc_ptr);

        return SUCCESS();
    } // load_resource_plugin
} // anonymous namespace

namespace irods
{
    // NOLINTBEGIN(cert-err58-cpp)
    const std::string EMPTY_RESC_HOST = "EMPTY_RESC_HOST";
    const std::string EMPTY_RESC_PATH = "EMPTY_RESC_PATH";

    /// definition of the resource interface
    const std::string RESOURCE_INTERFACE = "irods_resource_interface";

    /// special resource for local file system operations only
    const std::string LOCAL_USE_ONLY_RESOURCE = "LOCAL_USE_ONLY_RESOURCE";
    const std::string LOCAL_USE_ONLY_RESOURCE_VAULT = (get_irods_home_directory() / "LOCAL_USE_ONLY_RESOURCE_VAULT").c_str();
    const std::string LOCAL_USE_ONLY_RESOURCE_TYPE = "unixfilesystem";
    // NOLINTEND(cert-err58-cpp)

    error resource_manager::resolve(const std::string& _key, resource_ptr& _resc_ptr)
    {
        if (_key.empty()) {
            return ERROR(SYS_INVALID_INPUT_PARAM, "empty key");
        }

        if (resource_name_map_.has_entry(_key)) {
            _resc_ptr = resource_name_map_[_key];
            return SUCCESS();
        }

        return ERROR(SYS_RESC_DOES_NOT_EXIST, fmt::format("no resource found for name [{}]", _key));
    } // resolve

    error resource_manager::resolve(rodsLong_t _resc_id, resource_ptr& _resc_ptr)
    {
        if (resource_id_map_.has_entry(_resc_id)) {
            _resc_ptr = resource_id_map_[_resc_id];
            return SUCCESS();
        }

        return ERROR(SYS_RESC_DOES_NOT_EXIST, fmt::format("no resource found for name [{}]", _resc_id));
    } // resolve

    error resource_manager::validate_vault_path(
        const std::string& _physical_path,
        const rodsServerHost_t* _host,
        std::string& _vault_path)
    {
        // simple flag to state a resource matching the prop and value is found
        bool found = false;

        if (!_host) { // NOLINT(readability-implicit-bool-conversion)
            return ERROR(USER__NULL_INPUT_ERR, "empty server host");
        }

        if (resource_name_map_.empty()) {
            return ERROR(SYS_INVALID_INPUT_PARAM, "empty resource table");
        }

        // quick check on the path that it has something in it
        if (_physical_path.empty()) {
            return ERROR(SYS_INVALID_INPUT_PARAM, "empty property");
        }

        for (auto&& [name, resc_ptr] : resource_name_map_) {
            // get the host pointer from the resource
            rodsServerHost_t* svr_host{};
            error ret = resc_ptr->get_property<rodsServerHost_t*>(RESOURCE_HOST, svr_host);
            if (!ret.ok()) {
                PASS(ret);
            }

            // if this host matches the incoming host pointer then were good
            // otherwise continue searching
            if (svr_host != _host) {
                continue;
            }

            // query resource for the property value
            std::string path;
            ret = resc_ptr->get_property<std::string>(RESOURCE_PATH, path);

            // if we get a good parameter and do not match non-storage nodes with an empty physical path
            if (!ret.ok()) {
                const auto msg = fmt::format("failed to get vault path for resource [{}]", name);
                log_resc::error(PASSMSG(msg, ret).result());
            }

            // compare incoming value and stored value
            // one may be a subset of the other so compare both ways
            if (!path.empty() && _physical_path.find(path) != std::string::npos) {
                found = true;
                _vault_path = path;
            }
        }

        // did we find a resource?
        if (found) {
            return SUCCESS();
        }

        // TODO
        return ERROR(SYS_INVALID_INPUT_PARAM, fmt::format("failed to find resource for path [{}]", _physical_path));
    } // validate_vault_path

    void resource_manager::init_resource_from_database_row(const std::vector<std::string>& _row)
    {
        const auto& name = _row.at(1);
        const auto& type = _row.at(3);
        const auto& context = _row.at(14);

        resource_ptr resc_ptr;
        if (const auto ret = load_resource_plugin(resc_ptr, type, name, context); !ret.ok()) {
            irods::log(PASS(ret));
            return;
        }

        const auto& location = _row.at(5);
        if (irods::EMPTY_RESC_HOST != location) {
            const auto& zone_name = _row.at(2);
            rodsHostAddr_t addr{};
            std::strncpy(addr.hostAddr, location.c_str(), LONG_NAME_LEN);
            std::strncpy(addr.zoneName, zone_name.c_str(), NAME_LEN);

            rodsServerHost_t* host{};
            if (resolveHost(&addr, &host) < 0) {
                log_resc::info("[{}:{}] - resolveHost error for [{}]", __func__, __LINE__, addr.hostAddr);
            }

            resc_ptr->set_property<rodsServerHost_t*>(RESOURCE_HOST, host);
        }
        else {
            resc_ptr->set_property<rodsServerHost_t*>(RESOURCE_HOST, nullptr);
        }

        const rodsLong_t id = std::strtoll(_row.at(0).c_str(), 0, 0);
        resc_ptr->set_property<rodsLong_t>(RESOURCE_ID, id);
        resc_ptr->set_property<long>(RESOURCE_QUOTA, RESC_QUOTA_UNINIT);

        const auto& status = _row.at(12);
        resc_ptr->set_property(RESOURCE_STATUS, (status == RESC_DOWN) ? INT_RESC_STATUS_DOWN : INT_RESC_STATUS_UP);

        resc_ptr->set_property<std::string>(RESOURCE_FREESPACE,      _row.at(7));
        resc_ptr->set_property<std::string>(RESOURCE_ZONE,           _row.at(2));
        resc_ptr->set_property<std::string>(RESOURCE_NAME,           name);
        resc_ptr->set_property<std::string>(RESOURCE_LOCATION,       location);
        resc_ptr->set_property<std::string>(RESOURCE_TYPE,           type);
        resc_ptr->set_property<std::string>(RESOURCE_CLASS,          _row.at(4));
        resc_ptr->set_property<std::string>(RESOURCE_PATH,           _row.at(6));
        resc_ptr->set_property<std::string>(RESOURCE_INFO,           _row.at(8));
        resc_ptr->set_property<std::string>(RESOURCE_COMMENTS,       _row.at(9));
        resc_ptr->set_property<std::string>(RESOURCE_CREATE_TS,      _row.at(10));
        resc_ptr->set_property<std::string>(RESOURCE_MODIFY_TS,      _row.at(11));
        resc_ptr->set_property<std::string>(RESOURCE_CHILDREN,       _row.at(13));
        resc_ptr->set_property<std::string>(RESOURCE_CONTEXT,        context);
        resc_ptr->set_property<std::string>(RESOURCE_PARENT,         _row.at(15));
        resc_ptr->set_property<std::string>(RESOURCE_PARENT_CONTEXT, _row.at(16));

        resource_name_map_[name] = resc_ptr;
        resource_id_map_[id] = resc_ptr;
    } // init_resource_from_database_row

    error resource_manager::init_from_catalog(RsComm& _comm)
    {
        resource_name_map_.clear();

        constexpr char* query_str = "select "
                                    "RESC_ID, "             // 0
                                    "RESC_NAME, "           // 1
                                    "RESC_ZONE_NAME, "      // 2
                                    "RESC_TYPE_NAME, "      // 3
                                    "RESC_CLASS_NAME, "     // 4
                                    "RESC_LOC, "            // 5
                                    "RESC_VAULT_PATH, "     // 6
                                    "RESC_FREE_SPACE, "     // 7
                                    "RESC_INFO, "           // 8
                                    "RESC_COMMENT, "        // 9
                                    "RESC_CREATE_TIME, "    // 10
                                    "RESC_MODIFY_TIME, "    // 11
                                    "RESC_STATUS, "         // 12
                                    "RESC_CHILDREN, "       // 13
                                    "RESC_CONTEXT, "        // 14
                                    "RESC_PARENT, "         // 15
                                    "RESC_PARENT_CONTEXT";  // 16

        for (auto&& row : irods::experimental::query_builder{}.build(_comm, query_str)) {
            init_resource_from_database_row(row);
        }

        // Update child resource maps
        if (const auto proc_ret = init_parent_child_relationships(); !proc_ret.ok()) {
            return PASSMSG("init_parent_child_relationships failed.", proc_ret);
        }

        // gather the post disconnect maintenance operations
        error op_ret = gather_operations();
        if (!op_ret.ok()) {
            return PASSMSG("gather_operations failed.", op_ret);
        }

        // call start for plugins
        error start_err = start_resource_plugins();
        if (!start_err.ok()) {
            return PASSMSG("start_resource_plugins failed.", start_err);
        }

        // win!
        return SUCCESS();
    } // init_from_catalog

    error resource_manager::init_from_catalog(RsComm& _comm, const std::string_view _resc_name)
    {
        const auto gql = fmt::format("select "
                                     "RESC_ID, "             // 0
                                     "RESC_NAME, "           // 1
                                     "RESC_ZONE_NAME, "      // 2
                                     "RESC_TYPE_NAME, "      // 3
                                     "RESC_CLASS_NAME, "     // 4
                                     "RESC_LOC, "            // 5
                                     "RESC_VAULT_PATH, "     // 6
                                     "RESC_FREE_SPACE, "     // 7
                                     "RESC_INFO, "           // 8
                                     "RESC_COMMENT, "        // 9
                                     "RESC_CREATE_TIME, "    // 10
                                     "RESC_MODIFY_TIME, "    // 11
                                     "RESC_STATUS, "         // 12
                                     "RESC_CHILDREN, "       // 13
                                     "RESC_CONTEXT, "        // 14
                                     "RESC_PARENT, "         // 15
                                     "RESC_PARENT_CONTEXT "  // 16
                                     "where RESC_NAME = '{}'",
                                     _resc_name);

        for (auto&& row : irods::experimental::query_builder{}.build(_comm, gql)) {
            init_resource_from_database_row(row);
        }

        // Update child resource maps
        if (const auto err = init_parent_child_relationship(_resc_name); !err.ok()) {
            return PASSMSG( "init_parent_child_relationship failed.", err );
        }

        // Gather the post disconnect maintenance operations.
        if (const auto err = gather_operations(_resc_name); !err.ok()) {
            return PASSMSG("gather_operations failed.", err);
        }

        // Call start for the plugin.
        if (const auto err = start_resource_plugin(_resc_name); !err.ok()) {
            return PASSMSG("start_resource_plugin failed.", err);
        }

        return SUCCESS();
    } // init_from_catalog

    error resource_manager::shut_down_resources()
    {
        for (auto&& entry : resource_name_map_) {
            entry.second->stop_operation();
        }

        return SUCCESS();
    } // shut_down_resources

    error resource_manager::shut_down_resource(const std::string_view _resc_name)
    {
        const auto iter = resource_name_map_.find(std::string{_resc_name});

        if (iter == std::end(resource_name_map_)) {
            return ERROR(KEY_NOT_FOUND, fmt::format("No resource found by that name [{}]", _resc_name));
        }
        
        iter->second->stop_operation();

        return SUCCESS();
    } // shut_down_resource

    error resource_manager::shut_down_resource(rodsLong_t _resc_id)
    {
        const auto iter = resource_id_map_.find(_resc_id);

        if (iter == std::end(resource_id_map_)) {
            return ERROR(KEY_NOT_FOUND, fmt::format("No resource found by that id [{}]", _resc_id));
        }
        
        iter->second->stop_operation();

        return SUCCESS();
    } // shut_down_resource

    /// @brief create a list of resources who do not have parents ( roots )
    error resource_manager::get_root_resources(std::vector<std::string>& _resources)
    {
        for (auto&& [resc_name, resc_ptr] : resource_name_map_) {
            resource_ptr parent_ptr;

            if (auto ret = resc_ptr->get_parent(parent_ptr); !ret.ok()) {
                _resources.push_back(resc_name);
            }
        }

        return SUCCESS();
    } // get_root_resources

    error resource_manager::get_parent_name(resource_ptr _resc, std::string& _name)
    {
        std::string my_name;
        error ret = _resc->get_property<std::string>(RESOURCE_NAME, my_name);
        if (!ret.ok()) {
            return PASS(ret);
        }

        std::string parent_id_str;
        ret = _resc->get_property<std::string>(RESOURCE_PARENT, parent_id_str);
        if (!ret.ok()) {
            return PASS(ret);
        }

        if (parent_id_str.empty()) {
            std::stringstream msg;
            return ERROR(HIERARCHY_ERROR, "empty parent string");
        }

        rodsLong_t parent_id;
        ret = lexical_cast<rodsLong_t>(parent_id_str, parent_id);
        if (!ret.ok()) {
            return PASS(ret);
        }

        resource_ptr parent_resc;
        ret = resolve(parent_id, parent_resc);
        if (!ret.ok()) {
            return PASS(ret);
        }

        return parent_resc->get_property<std::string>(RESOURCE_NAME, _name);
    } // get_parent_name

    std::string resource_manager::get_hier_to_root_for_resc(std::string_view _resource_name)
    {
        if (_resource_name.empty()) {
            return {};
        }

        irods::hierarchy_parser hierarchy{_resource_name.data()};

        for (std::string parent_name = _resource_name.data(); !parent_name.empty();) {
            resource_ptr resc_ptr;

            if (const auto ret = resolve(parent_name, resc_ptr); !ret.ok()) {
                THROW(ret.code(), ret.result());
            }

            if (const auto ret = get_parent_name(resc_ptr, parent_name); !ret.ok()) {
                if (HIERARCHY_ERROR == ret.code()) {
                    break;
                }

                THROW(ret.code(), ret.result());
            }

            if (!parent_name.empty()) {
                hierarchy.add_parent(parent_name);
            }
        }

        return hierarchy.str();
    } // get_hier_to_root_for_resc

    error resource_manager::get_hier_to_root_for_resc(const std::string& _resc_name, std::string& _hierarchy)
    {
        _hierarchy = _resc_name;
        std::string parent_name = _resc_name;

        resource_ptr resc_ptr;

        while (!parent_name.empty()) {
            error ret = resolve(parent_name, resc_ptr);
            if (!ret.ok()) {
                return PASS(ret);
            }

            ret = get_parent_name(resc_ptr, parent_name);
            if (!ret.ok()) {
                if (HIERARCHY_ERROR == ret.code()) {
                    break;
                }

                return PASS(ret);
            }

            if (!parent_name.empty()) {
                _hierarchy = parent_name;
                _hierarchy += irods::hierarchy_parser::delimiter();
                _hierarchy += _hierarchy;
            }
        }

        return SUCCESS();
    } // get_hier_to_root_for_resc

    error resource_manager::gather_leaf_bundle_for_child(const std::string& _resc_name, leaf_bundle_t& _bundle)
    {
        resource_ptr resc_ptr;
        error ret = resolve(_resc_name, resc_ptr);
        if (!ret.ok()) {
            return PASS(ret);
        }

        if (resc_ptr->num_children() > 0) {
            // still more children to traverse
            std::vector<std::string> children;
            resc_ptr->children(children);

            for (std::size_t idx = 0; idx < children.size(); ++idx) {
                ret = gather_leaf_bundle_for_child(children[idx], _bundle);
                if (!ret.ok()) {
                    return PASS(ret);
                }
            }
        }
        else {
            // we have found a leaf
            rodsLong_t resc_id;
            ret = resc_ptr->get_property<rodsLong_t>(RESOURCE_ID, resc_id);
            if (!ret.ok()) {
                return PASS(ret);
            }

            _bundle.push_back(resc_id);
        }

        return SUCCESS();
    } // gather_leaf_bundle_for_child

    std::vector<resource_manager::leaf_bundle_t>
    resource_manager::gather_leaf_bundles_for_resc(const std::string& _resource_name)
    {
        resource_ptr resc_ptr;
        error err = resolve(_resource_name, resc_ptr);
        if (!err.ok()) {
            THROW(err.code(), err.result());
        }

        std::vector<std::string> children;
        resc_ptr->children(children);

        std::vector<leaf_bundle_t> ret;
        ret.resize(children.size());

        for (decltype(children.size()) i = 0; i < children.size(); ++i) {
            err = gather_leaf_bundle_for_child(children[i], ret[i]);

            if (!err.ok()) {
                THROW(err.code(), err.result());
            }

            std::sort(std::begin(ret[i]), std::end(ret[i]));
        }

        return ret;
    } // gather_leaf_bundle_for_resc

    // given a type, load up a resource plugin
    error resource_manager::init_from_type(const std::string& _type,
                                           const std::string& _key,
                                           const std::string& _inst,
                                           const std::string& _ctx,
                                           resource_ptr& _resc)
    {
        // create the resource and add properties for column values
        error ret = load_resource_plugin(_resc, _type, _inst, _ctx);
        if (!ret.ok()) {
            return PASSMSG("Failed to load Resource Plugin", ret);
        }

        resource_name_map_[_key] = _resc;

        return SUCCESS();
    } // init_from_type

    error resource_manager::init_parent_child_relationships()
    {
        for (auto& entry : resource_name_map_) {
            init_parent_child_relationship_impl(entry.first, entry.second);
        }

        return SUCCESS();
    } // init_parent_child_relationships

    error resource_manager::init_parent_child_relationship(const std::string_view _resc_name)
    {
        auto iter = resource_name_map_.find(std::string{_resc_name});

        if (std::end(resource_name_map_) == iter) {
            return ERROR(KEY_NOT_FOUND, fmt::format("No resource found by that name [{}]", _resc_name));
        }

        init_parent_child_relationship_impl(iter->first, iter->second);

        return SUCCESS();
    } // init_parent_child_relationship

    void resource_manager::init_parent_child_relationship_impl(
        const std::string_view _resc_name,
        resource_ptr _resc_ptr)
    {
        std::string parent_id_str;
        error ret = _resc_ptr->get_property<std::string>(RESOURCE_PARENT, parent_id_str);
        if (!ret.ok() || parent_id_str.empty()) {
            return;
        }

        rodsLong_t parent_id = 0;
        ret = lexical_cast<rodsLong_t>(parent_id_str, parent_id);
        if (!ret.ok()) {
            log_resc::error(PASS(ret).result());
            return;
        }

        if (!resource_id_map_.has_entry(parent_id)) {
            log_resc::error("Invalid parent resource id [{}] for resource [{}].", parent_id, _resc_name);
            return;
        }

        resource_ptr parent_resc = resource_id_map_[parent_id];

        std::string parent_child_context;
        _resc_ptr->get_property<std::string>(RESOURCE_PARENT_CONTEXT, parent_child_context);

        parent_resc->add_child(std::string{_resc_name}, parent_child_context, _resc_ptr);
        _resc_ptr->set_parent(parent_resc);

        log_resc::debug("{} - add [{}][{}] to [{}]", __func__, _resc_name, parent_child_context, parent_id);
    } // init_parent_child_relationship_impl

    void resource_manager::add_child_to_resource(rodsLong_t _parent_resc_id, rodsLong_t _child_resc_id)
    {
        auto parent_resc_ptr = resource_id_map_[_parent_resc_id];
        std::string resc_name;
        if (const auto err = parent_resc_ptr->get_property<std::string>(RESOURCE_NAME, resc_name); !err.ok()) {
            THROW(err.code(), err.result());
        }

        auto child_resc_ptr = resource_id_map_[_child_resc_id];
        std::string child_resc_name;
        if (const auto err = child_resc_ptr->get_property<std::string>(RESOURCE_NAME, resc_name); !err.ok()) {
            THROW(err.code(), err.result());
        }

        std::string parent_child_context;
        auto err = child_resc_ptr->get_property<std::string>(RESOURCE_PARENT_CONTEXT, parent_child_context);
        log_resc::info(err.result());

        err = parent_resc_ptr->add_child(std::string{child_resc_name}, parent_child_context, child_resc_ptr); // TODO Can throw!!!
        log_resc::info(err.result());
        err = child_resc_ptr->set_parent(parent_resc_ptr); // TODO Can throw!!!
        log_resc::info(err.result());
        err = child_resc_ptr->set_property<std::string>(RESOURCE_PARENT, std::to_string(_parent_resc_id)); // TODO Can throw!!!
        log_resc::info(err.result());
    } // add_child_to_resource

    void resource_manager::add_child_to_resource(
        const std::string_view _parent_resc_name,
        const std::string_view _child_resc_name)
    {
        log_resc::info(">>>>>>>>>>>>> IN {}", __func__);
        log_resc::info("LOOKING UP PARENT RESC PTR USING NAME [{}]", _parent_resc_name);
        auto parent_resc_ptr = resource_name_map_[std::string{_parent_resc_name}];

        log_resc::info("LOOKING UP CHILD RESC PTR USING NAME [{}]", _child_resc_name);
        const std::string child_resc_name{_child_resc_name};
        auto child_resc_ptr = resource_name_map_[child_resc_name];

        log_resc::info("GETTING RESOURCE_PARENT_CONTEXT KEY FROM CHILD RESC PTR");
        std::string parent_child_context;
        auto err = child_resc_ptr->get_property<std::string>(RESOURCE_PARENT_CONTEXT, parent_child_context);
        log_resc::info("child_resc_ptr->get_property<std::string>(RESOURCE_PARENT_CONTEXT) ::: value = [{}] ::: {}", parent_child_context, err.result());

        log_resc::info("GETTING RESOURCE_ID KEY FROM CHILD RESC PTR");
        rodsLong_t parent_resc_id;
        err = parent_resc_ptr->get_property<rodsLong_t>(RESOURCE_ID, parent_resc_id);
        log_resc::info("child_resc_ptr->get_property<std::string>(RESOURCE_ID) ::: value = [{}] ::: {}", parent_resc_id, err.result());

        log_resc::info("UPDATING PARENT / CHILD RESC POINTERS");
        err = parent_resc_ptr->add_child(child_resc_name, parent_child_context, child_resc_ptr); // TODO Can throw!!!
        log_resc::info(err.result());
        err = child_resc_ptr->set_parent(parent_resc_ptr); // TODO Can throw!!!
        log_resc::info(err.result());
        err = child_resc_ptr->set_property<std::string>(RESOURCE_PARENT, std::to_string(parent_resc_id)); // TODO Can throw!!!
        log_resc::info(err.result());
        log_resc::info("DONE");
    } // add_child_to_resource

    void resource_manager::remove_child_from_resource(rodsLong_t _parent_resc_id, rodsLong_t _child_resc_id)
    {
        auto parent_resc_ptr = resource_id_map_[_parent_resc_id];
        std::string resc_name;
        if (const auto err = parent_resc_ptr->get_property<std::string>(RESOURCE_NAME, resc_name); !err.ok()) {
            THROW(err.code(), err.result());
        }

        auto child_resc_ptr = resource_id_map_[_child_resc_id];
        std::string child_resc_name;
        if (const auto err = child_resc_ptr->get_property<std::string>(RESOURCE_NAME, resc_name); !err.ok()) {
            THROW(err.code(), err.result());
        }

        parent_resc_ptr->remove_child(std::string{child_resc_name});
        child_resc_ptr->set_parent(nullptr);
        child_resc_ptr->set_property<std::string>(RESOURCE_PARENT, ""); // TODO Can throw!!!
    } // add_child_to_resource

    void resource_manager::remove_child_from_resource(
        const std::string_view _parent_resc_name,
        const std::string_view _child_resc_name)
    {
        auto parent_resc_ptr = resource_name_map_[std::string{_parent_resc_name}];

        const std::string child_resc_name{_child_resc_name};
        auto child_resc_ptr = resource_name_map_[child_resc_name];

        parent_resc_ptr->remove_child(child_resc_name);
        child_resc_ptr->set_parent(nullptr);
        child_resc_ptr->set_property<std::string>(RESOURCE_PARENT, ""); // TODO Can throw!!!
    } // add_child_to_resource

    void resource_manager::erase_resource(rodsLong_t _resc_id)
    {
        if (!resource_id_map_.has_entry(_resc_id)) {
            return;
        }

        auto resc_ptr = resource_id_map_[_resc_id];
        std::string resc_name;

        if (const error err = resc_ptr->get_property<std::string>(RESOURCE_NAME, resc_name); !err.ok()) {
            THROW(err.code(), err.result());
        }

        resource_name_map_.erase(resc_name);
        resource_id_map_.erase(_resc_id);
    } // erase_resource

    void resource_manager::erase_resource(const std::string_view _resc_name)
    {
        if (!resource_name_map_.has_entry(_resc_name.data())) {
            return;
        }

        auto resc_ptr = resource_name_map_[_resc_name.data()];
        rodsLong_t resc_id = 0;

        if (const error err = resc_ptr->get_property<rodsLong_t>(RESOURCE_ID, resc_id); !err.ok()) {
            THROW(err.code(), err.result());
        }

        resource_id_map_.erase(resc_id);
        resource_name_map_.erase(_resc_name.data());
    } // erase_resource

    // print the list of local resources out to stderr
    void resource_manager::print_local_resources()
    {
        for (auto&& [name, ptr] : resource_name_map_) {
            std::string path;
            if (auto err = ptr->get_property<std::string>(RESOURCE_PATH, path); !err.ok()) {
                continue;
            }

            std::string location;
            if (auto err = ptr->get_property<std::string>(RESOURCE_LOCATION, location); !err.ok()) {
                continue;
            }

            if ("localhost" == location) {
                log_resc::info("   RescName: {}, VaultPath: {}\n", name, path);
            }
        }
    } // print_local_resources

    // gather the post disconnect maintenance operations from the resource plugins
    error resource_manager::gather_operations()
    {
        // vector of already processed resources
        std::vector<std::string> proc_vec;

        // iterate over all of the resources
        for (auto&& [name, resc_ptr] : resource_name_map_) {
            // Skip resources that have already been processed.
            auto itr = std::find(proc_vec.begin(), proc_vec.end(), name);
            if (proc_vec.end() != itr) {
                continue;
            }

            // vector which will hold this 'top level resource' ops
            std::vector<pdmo_type> resc_ops;

            // cache the parent operator
            pdmo_type pdmo_op;
            error pdmo_err = resc_ptr->post_disconnect_maintenance_operation(pdmo_op);
            if (pdmo_err.ok()) {
                resc_ops.push_back(pdmo_op);
            }

            // mark this resource done
            proc_vec.push_back(name);

            // dive if children are present
            std::string child_str;
            error child_err = resc_ptr->get_property<std::string>(RESOURCE_CHILDREN, child_str);
            if (child_err.ok() && !child_str.empty()) {
                gather_operations_recursive(child_str, proc_vec, resc_ops);
            }

            // if we got ops, add vector of ops to mgr's vector
            if (!resc_ops.empty()) {
                maintenance_operations_.push_back(resc_ops);
            }
        }

        return SUCCESS();
    } // gather_operations

    error resource_manager::gather_operations(const std::string_view _resc_name)
    {
        const auto iter = resource_name_map_.find(_resc_name.data());

        if (iter == std::end(resource_name_map_)) {
            return ERROR(SYS_RESC_DOES_NOT_EXIST, fmt::format("Resource [{}] does not exist in map.", _resc_name));
        }

        auto resc_ptr = iter->second;

        // Will hold this "top level resource" operations.
        std::vector<pdmo_type> resc_ops;

        // Cache the parent operator.
        pdmo_type pdmo_op;
        if (const auto err = resc_ptr->post_disconnect_maintenance_operation(pdmo_op); err.ok()) {
            resc_ops.push_back(pdmo_op);
        }

        // Handle child resources.
        std::string child_str;
        if (const auto err = resc_ptr->get_property<std::string>(RESOURCE_CHILDREN, child_str);
            err.ok() && !child_str.empty())
        {
            std::vector<std::string> proc_vec;
            proc_vec.push_back(std::string{_resc_name});
            gather_operations_recursive(child_str, proc_vec, resc_ops);
        }

        // If we got operations, add them to the resource manager's vector.
        if (!resc_ops.empty()) {
            maintenance_operations_.push_back(resc_ops);
        }

        return SUCCESS();
    } // gather_operations

    /// lower level recursive call to gather the post disconnect maintenance operations from the resources,
    /// in breadth first order
    error resource_manager::gather_operations_recursive(
        const std::string& _children,
        std::vector<std::string>& _processed_resources,
        std::vector<pdmo_type>& _gathered_ops)
    {
        // create a child parser to traverse the list
        children_parser parser;
        parser.set_string(_children);
        children_parser::children_map_t children_list;
        error ret = parser.list(children_list);
        if (!ret.ok()) {
            return PASSMSG("gather_operations_recursive failed.", ret);
        }

        // iterate over all of the children, cache the operators
        for (auto itr = children_list.begin(); itr != children_list.end(); ++itr) {
            const std::string& child = itr->first;

            // lookup the child resource pointer
            resource_ptr resc;
            error get_err = resource_name_map_.get(child, resc);
            if (!get_err.ok()) {
                return ERROR(SYS_INVALID_INPUT_PARAM, fmt::format("failed to get resource for key [{}]", child));
            }

            // cache operation if there is one
            pdmo_type pdmo_op;
            error pdmo_ret = resc->post_disconnect_maintenance_operation(pdmo_op);
            if (pdmo_ret.ok()) {
                _gathered_ops.push_back(pdmo_op);
            }

            // mark this child as done
            _processed_resources.push_back(child);
        }

        // iterate over all of the children again, recurse if they have more children
        for (auto itr = children_list.begin(); itr != children_list.end(); ++itr) {
            const std::string& child = itr->first;

            // lookup the child resource pointer
            resource_ptr resc;
            error get_err = resource_name_map_.get(child, resc);
            if (!get_err.ok()) {
                return ERROR(SYS_INVALID_INPUT_PARAM, fmt::format("failed to get resource fro key [{}]", child));
            }

            std::string child_str;
            error child_err = resc->get_property<std::string>(RESOURCE_CHILDREN, child_str);
            if (child_err.ok() && !child_str.empty()) {
                gather_operations_recursive(child_str, _processed_resources, _gathered_ops);
            }
        }

        return SUCCESS();
    } // gather_operations_recursive

    // call the start op on the resource plugins
    error resource_manager::start_resource_plugins()
    {
        for (auto&& [name, resc_ptr] : resource_name_map_) {
            // if any resources need a pdmo, return true;
            error ret = resc_ptr->start_operation();
            if (!ret.ok()) {
                irods::log(ret);
            }
        }

        return SUCCESS();
    } // start_resource_operations

    error resource_manager::start_resource_plugin(const std::string_view _resc_name)
    {
        if (!resource_name_map_.has_entry(_resc_name.data())) {
            return ERROR(KEY_NOT_FOUND, fmt::format("Resource [{}] not in map.", _resc_name));
        }

        if (const auto err = resource_name_map_[_resc_name.data()]->start_operation(); !err.ok()) {
            log_resc::error(err.result());
        }

        return SUCCESS();
    } // start_resource_plugin

    // exec the pdmos ( post disconnect maintenance operations ) in order
    bool resource_manager::need_maintenance_operations()
    {
        for (auto&& [name, resc_ptr] : resource_name_map_) {
            bool value = false;
            resc_ptr->need_post_disconnect_maintenance_operation(value);

            // if any resources need a pdmo, return true;
            if (value) {
                return value;
            }
        }

        return false;
    } // need_maintenance_operations

    // exec the pdmos ( post disconnect maintenance operations ) in order
    int resource_manager::call_maintenance_operations(RcComm* _comm)
    {
        int result = 0;

        // iterate through op vectors
        for (auto&& m_ops : maintenance_operations_) {
            // iterate through ops
            for (auto&& op : m_ops) {
                // call the op
                try {
                    error ret = op(_comm);
                    if (!ret.ok()) {
                        log(PASSMSG("resource_manager::call_maintenance_operations - op failed", ret));
                        result = ret.code();
                    }
                }
                catch (const boost::bad_function_call&) {
                    log_resc::error("maintenance operation threw boost::bad_function_call");
                    result = SYS_INTERNAL_ERR;
                }
            }
        }

        return result;
    } // call_maintenance_operations

    // construct a vector of all resource hierarchies in the system
    // throws irods::exception
    std::vector<std::string> resource_manager::get_all_resc_hierarchies()
    {
        std::vector<std::string> hier_list;

        for (const auto& [name, resc_ptr] : resource_name_map_) {
            if (resc_ptr->num_children() > 0) {
                continue;
            }

            rodsLong_t leaf_id = 0;
            error err = resc_ptr->get_property<rodsLong_t>(RESOURCE_ID, leaf_id);
            if (!err.ok()) {
                THROW(err.code(), err.result());
            }

            std::string curr_hier;
            err = leaf_id_to_hier(leaf_id, curr_hier);
            if (!err.ok()) {
                THROW(err.code(), err.result());
            }

            hier_list.push_back(curr_hier);
        }

        return hier_list;
    } // get_all_resc_hierarchies

    rodsLong_t resource_manager::hier_to_leaf_id(std::string_view _hierarchy)
    {
        if (_hierarchy.empty()) {
            THROW(HIERARCHY_ERROR, "empty hierarchy string");
        }

        const std::string leaf = irods::hierarchy_parser{_hierarchy.data()}.last_resc();
        if (!resource_name_map_.has_entry(leaf)) {
            THROW(SYS_RESC_DOES_NOT_EXIST, leaf);
        }

        rodsLong_t id = 0;
        const resource_ptr resc = resource_name_map_[leaf];
        if (const error ret = resc->get_property<rodsLong_t>(RESOURCE_ID, id); !ret.ok()) {
            THROW(ret.code(), ret.result());
        }
        return id;
    } // hier_to_leaf_id

    error resource_manager::hier_to_leaf_id(const std::string& _hier, rodsLong_t& _id)
    {
        if (_hier.empty()) {
            return ERROR(HIERARCHY_ERROR, "empty hierarchy string");
        }

        hierarchy_parser p;
        p.set_string(_hier);

        std::string leaf;
        p.last_resc(leaf);

        if (!resource_name_map_.has_entry(leaf)) {
            return ERROR(SYS_RESC_DOES_NOT_EXIST, leaf);
        }

        resource_ptr resc = resource_name_map_[leaf];

        rodsLong_t id = 0;
        error ret = resc->get_property<rodsLong_t>(RESOURCE_ID, id);
        if (!ret.ok()) {
            return PASS(ret);
        }

        _id = id;

        return SUCCESS();
    } // hier_to_leaf_id

    std::string resource_manager::leaf_id_to_hier(const rodsLong_t _leaf_resource_id)
    {
        if (!resource_id_map_.has_entry(_leaf_resource_id)) {
            THROW(SYS_RESC_DOES_NOT_EXIST, fmt::format("invalid resource id: {}", _leaf_resource_id));
        }

        resource_ptr resc = resource_id_map_[_leaf_resource_id];

        std::string leaf_name;
        if (const error ret = resc->get_property<std::string>(RESOURCE_NAME, leaf_name); !ret.ok()) {
            THROW(ret.code(), ret.result());
        }

        irods::hierarchy_parser parser{leaf_name};

        resc->get_parent(resc);
        while(resc.get()) {
            std::string name;

            if (const error ret = resc->get_property<std::string>(RESOURCE_NAME, name); !ret.ok()) {
                THROW(ret.code(), ret.result());
            }

            parser.add_parent(name);

            resc->get_parent(resc);
        }

        return parser.str();
    } // leaf_id_to_hier

    error resource_manager::leaf_id_to_hier(const rodsLong_t& _id, std::string& _hier)
    {
        if (!resource_id_map_.has_entry(_id)) {
            return ERROR(SYS_RESC_DOES_NOT_EXIST, fmt::format("Invalid resource id [{}]", _id));
        }

        resource_ptr resc = resource_id_map_[_id];

        std::string hier;
        error ret = resc->get_property<std::string>(RESOURCE_NAME, hier);
        if (!ret.ok()) {
            return PASS(ret);
        }

        resc->get_parent(resc);
        while (resc.get()) {
            std::string name;
            error ret = resc->get_property<std::string>(RESOURCE_NAME, name);
            if (!ret.ok()) {
                return PASS(ret);
            }

            hier.insert(0, ";");
            hier.insert(0, name);

            resc->get_parent(resc);
        }

        _hier = hier;

        return SUCCESS();
    } // leaf_id_to_hier

    error resource_manager::resc_id_to_name(const rodsLong_t& _id, std::string& _name)
    {
        // parent name might be 'empty'
        if (!_id) {
            return SUCCESS();
        }

        if (!resource_id_map_.has_entry(_id)) {
            return ERROR(SYS_RESC_DOES_NOT_EXIST, fmt::format("Invalid resource id [{}]", _id));
        }

        resource_ptr resc = resource_id_map_[_id];

        std::string hier;
        error ret = resc->get_property<std::string>(RESOURCE_NAME, _name);
        if (!ret.ok()) {
            return PASS(ret);
        }

        return SUCCESS();
    } // resc_id_to_name

    error resource_manager::resc_id_to_name(const std::string& _id_str, std::string& _name)
    {
        // parent name might be 'empty'
        if("0" == _id_str || _id_str.empty()) {
            return SUCCESS();
        }

        rodsLong_t resc_id = 0;
        error ret = lexical_cast<rodsLong_t>(_id_str, resc_id);
        if (!ret.ok()) {
            return PASS(ret);
        }

        if (!resource_id_map_.has_entry(resc_id)) {
            return ERROR(SYS_RESC_DOES_NOT_EXIST, fmt::format("Invalid resource id [{}]", _id_str));
        }

        resource_ptr resc = resource_id_map_[resc_id];

        std::string hier;
        ret = resc->get_property<std::string>(RESOURCE_NAME, _name);
        if (!ret.ok()) {
            return PASS(ret);
        }

        return SUCCESS();
    } // resc_id_to_name

    std::string resource_manager::resc_id_to_name(const rodsLong_t& _id)
    {
        if (!_id) {
            return {};
        }

        if (!resource_id_map_.has_entry(_id)) {
            THROW(SYS_RESC_DOES_NOT_EXIST, fmt::format("Invalid resource id [{}]", _id));
        }

        std::string resource_name;
        const resource_ptr resc = resource_id_map_[_id];
        if (const error ret = resc->get_property<std::string>(RESOURCE_NAME, resource_name); !ret.ok()) {
            THROW(ret.code(), ret.result());
        }
        return resource_name;
    } // resc_id_to_name

    std::string resource_manager::resc_id_to_name(std::string_view _id)
    {
        if (_id.empty()) {
            return {};
        }

        rodsLong_t id = 0;
        if (const error ret = lexical_cast<rodsLong_t>(_id, id); !ret.ok()) {
            THROW(ret.code(), ret.result());
        }

        return resc_id_to_name(id);
    } // resc_id_to_name

    error resource_manager::is_coordinating_resource(const std::string& _resc_name, bool& _ret)
    {
        if (!resource_name_map_.has_entry(_resc_name)) {
            return ERROR(SYS_RESC_DOES_NOT_EXIST, _resc_name);
        }

        resource_ptr resc = resource_name_map_[_resc_name];

        _ret = resc->num_children() > 0;

        return SUCCESS();
    } // is_coordinating_resource

    bool resource_manager::is_coordinating_resource(const std::string& _resc_name)
    {
        if (!resource_name_map_.has_entry(_resc_name)) {
            THROW(SYS_RESC_DOES_NOT_EXIST, _resc_name);
        }

        resource_ptr resc = resource_name_map_[_resc_name];

        return resc->num_children() > 0;
    } // is_coordinating_resource
} // namespace irods

