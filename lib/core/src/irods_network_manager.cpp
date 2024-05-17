// =-=-=-=-=-=-=-
#include "irods/irods_network_manager.hpp"
#include "irods/irods_load_plugin.hpp"
// TODO(#7779): Revisit logging in network manager
//#include "irods/irods_log.hpp"

#include <shared_mutex>
#include <string>
#include <mutex>

#include <fmt/format.h>

namespace irods
{
    // =-=-=-=-=-=-=-
    // network manager singleton
    network_manager netwk_mgr{};

    // =-=-=-=-=-=-=-
    // public - Constructor
    network_manager::network_manager() = default;

    // =-=-=-=-=-=-=-
    // public - Copy Constructor
    network_manager::network_manager(const network_manager& _rhs)
        : plugins_(_rhs.plugins_), plugins_old_(_rhs.plugins_old_) {}

    // =-=-=-=-=-=-=-
    // public - Destructor
    network_manager::~network_manager() = default;

    static error load_network_plugin(
        network_ptr&       _plugin,
        const std::string& _plugin_name,
        const std::string& _inst_name,
        const std::string& _context)
    {
        // call generic plugin loader
        network* net = 0;
        error ret = load_plugin<network>(
                        net,
                        _plugin_name,
                        KW_CFG_PLUGIN_TYPE_NETWORK,
                        _inst_name,
                        _context);
        if (ret.ok() && (net != nullptr)) {
            _plugin.reset(net);
            return SUCCESS();

        }
        else {
            return PASS(ret);
        }

    } // load_network_plugin

    // =-=-=-=-=-=-=-
    // public - return a network plugin, loading if necessary
    error network_manager::get_plugin(
        const int&         _proc_type,
        const std::string& _type,
        const std::string& _inst,
        const std::string& _ctx,
        network_ptr&       _net)
    {
        plugin_map_key key(_proc_type, _type, _inst, _ctx);

        // look for existing plugin
        {
            // we're going to read the plugins table
            // acquire shared lock so writers block
            std::shared_lock lock(table_lock_);

            auto plugin_itr_ = plugins_.find(key);
            if (plugin_itr_ != plugins_.end()) {
                _net = plugin_itr_->second;
                return SUCCESS();
            }
        }

        // plugin not loaded yet
        {
            // we're (probably) going to write to the plugins table
            // acquire unique lock so other accessors block
            std::unique_lock lock(table_lock_);

            // check table again in case the plugin was added while we were waiting on the lock
            auto plugin_itr_ = plugins_.find(key);
            if (plugin_itr_ != plugins_.end()) {
                _net = plugin_itr_->second;
                return SUCCESS();
            }

            std::string type = _type;
            type += (CLIENT_PT == _proc_type) ? "_client" : "_server";

            // load plugin
            network_ptr ptr;
            error ret = load_network_plugin(ptr, type, _inst, _ctx);
            if (!ret.ok()) {
                return PASSMSG("Failed to load network plugin", ret);
            }

            // insert plugin into table
            bool was_inserted = false;
            std::tie(plugin_itr_, was_inserted) = plugins_.try_emplace(key, ptr);
            if (!was_inserted) {
                return ERROR(SYS_INTERNAL_ERR, "Plugin already in table despite lock");
            }

            _net = plugin_itr_->second;

            // insert into legacy plugins table (for deprecated functions)
            ret = plugins_old_.set(_inst, ptr);
#if 0 // TODO(#7779): Revisit logging in network manager
            if (!ret.ok()) {
                log(LOG_WARNING, ret.result());
            }
            else {
                // verify insertion into legacy table
                network_ptr temp_ptr;
                ret = plugins_old_.get(_inst, temp_ptr);
                if (!ret.ok()) {
                    log(LOG_WARNING, ret.result());
                }
                else if (_net != temp_ptr) {
                    log(LOG_WARNING, "legacy plugins table doesn't match regular plugins table");
                }
            }
#endif
        }

        return SUCCESS();
    }

    // =-=-=-=-=-=-=-
    // deprecated - retrieve a network plugin given its key
    error network_manager::resolve(const std::string& _key, network_ptr& _value)
    {
        if (_key.empty()) {
            return ERROR(SYS_INVALID_INPUT_PARAM, "empty key");
        }

        // we're going to read the plugins table
        // acquire shared lock so writers block
        std::shared_lock lock(table_lock_);
        auto plugin_itr_ = plugins_old_.find(_key);

        if (plugin_itr_ == plugins_old_.end()) {
            return ERROR(KEY_NOT_FOUND, fmt::format("no network plugin found for name [{}]", _key));
        }

        _value = plugin_itr_->second;
        return SUCCESS();

    } // resolve

    // =-=-=-=-=-=-=-
    // deprecated - given a type, load up a network plugin
    error network_manager::init_from_type(
        const int&         _proc_type,
        const std::string& _type,
        const std::string& _key,
        const std::string& _inst,
        const std::string& _ctx,
        network_ptr&       _net)
    {
        if (_key != _inst) {
            return ERROR(SYS_INVALID_INPUT_PARAM, "key and instance name must match");
        }

        if (_key.empty()) {
            return ERROR(SYS_INVALID_INPUT_PARAM, "empty key");
        }

        std::string type = _type;
        type += (CLIENT_PT == _proc_type) ? "_client" : "_server";

        // we're (probably) going to write to the plugins table
        // acquire unique lock so other accessors block
        std::unique_lock lock(table_lock_);

        // =-=-=-=-=-=-=-
        // create the network plugin and add it to the table
        network_ptr ptr;
        error ret = load_network_plugin(ptr, type, _inst, _ctx);
        if (!ret.ok()) {
            return PASSMSG("Failed to load network plugin", ret);
        }

        plugins_old_[_inst] = ptr;

        _net = plugins_old_[_inst];

        return SUCCESS();

    } // init_from_type

}; // namespace irods
