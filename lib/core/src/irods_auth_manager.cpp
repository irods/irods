#include "irods/irods_auth_manager.hpp"

#include "irods/irods_auth_plugin.hpp"

#include <fmt/format.h>

namespace
{
    // function to load and return an initialized auth plugin
    auto load_auth_plugin(irods::auth_ptr& _plugin,
                          const std::string& _name,
                          const std::string& _instance,
                          const std::string& _context) -> irods::error
    {
        irods::auth* auth = nullptr;

        constexpr auto& type = irods::PLUGIN_TYPE_AUTHENTICATION;
        if (const auto err = irods::load_plugin<irods::auth>(auth, _name, type, _instance, _context);
            !err.ok()) {
            return PASSMSG(fmt::format("Failed to load plugin: \"{}\".", _name), err);
        }

        if (!auth) {
            return ERROR(SYS_INVALID_INPUT_PARAM, "Invalid auth plugin.");
        }

        _plugin.reset(auth);

        return SUCCESS();
    } // load_auth_plugin
} // anonymous namespace

namespace irods {
// =-=-=-=-=-=-=-
// globally available auth manager
    auth_manager auth_mgr;

    auth_manager::auth_manager( void ) {
        // TODO - stub
    }

    auth_manager::auth_manager(
        const auth_manager& _rhs ) {
        plugins_ = _rhs.plugins_;
    }

    auth_manager::~auth_manager() {
        // TODO - stub
    }

    auto auth_manager::resolve(const std::string& _key, irods::auth_ptr& _value) -> irods::error
    {
        if (_key.empty()) {
            return ERROR(SYS_INVALID_INPUT_PARAM, "Empty plugin name.");
        }

        if (!plugins_.has_entry(_key)) {
            return ERROR(SYS_INVALID_INPUT_PARAM, fmt::format(
                "No auth plugin found for name: \"{}\".", _key));
        }

        _value = plugins_[_key];

        return SUCCESS();
    } // resolve

    auto auth_manager::init_from_type(const int&         _proc_type,
                                      const std::string& _type,
                                      const std::string& _key,
                                      const std::string& _inst,
                                      const std::string& _ctx,
                                      irods::auth_ptr&   _rtn_auth) -> irods::error
    {
        if (_key.empty()) {
            return ERROR(SYS_INVALID_INPUT_PARAM, "Empty plugin name.");
        }

        const std::string type = _type + (CLIENT_PT == _proc_type ? "_client" : "_server");

        irods::auth_ptr auth;
        if (const auto err = load_auth_plugin(auth, type, _inst, _ctx); !err.ok()) {
            return PASSMSG("Failed to load auth plugin.", err);
        }

        plugins_[_key] = auth;
        _rtn_auth = plugins_[_key];

        return SUCCESS();
    } // init_from_type
} // namespace irods
