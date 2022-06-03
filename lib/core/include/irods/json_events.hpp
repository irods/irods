#ifndef IRODS_JSON_EVENTS_HPP
#define IRODS_JSON_EVENTS_HPP

/// \file

#include <nlohmann/json.hpp>

#include <vector>
#include <string>
#include <unordered_map>

namespace irods::experimental::json_events
{
    /// \brief The string used for the catch-all hooks that are called on every configuration change.
    /// \see hook
    extern const std::string KW_ALL_KEYS;

    /// \brief Hooks are functions called with the pathname of the json value that
    /// has changed, as well as the new operation performed, removal versus
    /// replacement versus insertion etc.
    ///
    /// Hooks are invoked in two stages for each entry in a list of json patches.
    ///
    /// The first is the \p KW_ALL_KEYS stage, where hooks that are run for every
    /// configuration change occurs. The second is based on the specific json
    /// path of the changed configuration value.
    ///
    /// For example, if you change the delay server's sleep duration, the
    /// \p KW_ALL_KEYS hooks will be run first in order of addition, followed by
    /// the hooks associated with
    /// "/advanced_settings/delay_server_sleep_time_in_seconds", also in order
    /// of addition.
    using hook = std::function<void(const std::string&, const nlohmann::json&)>;

    namespace detail
    {
        std::vector<hook>& get_hooks();
        std::unordered_multimap<std::string, std::size_t>& get_edge_sets();
    } // namespace detail

    /// \brief Adds a hook to be run whenever a key on the list of hooks changes.
    ///        \see hook
    ///
    /// \param [in] hook_triggers An array or container of json paths to match, this
    ///             container must support begin and end iterators.
    ///
    ///             The special value \p KW_ALL_KEYS matches any property that has changed and gets run first
    /// \param [in] hook The function to run when the json path matches.
    ///
    /// \b Example
    /// \code{.cpp}
    /// namespace je = irods::experimental::json_events;
    ///
    /// je::add_hook({je::KW_ALL_KEYS}, [&](const std::string& json_path,
    ///                                     const nlohmann::json& json_patch_op)
    /// {
    ///     if(json_path.substr(0,29) != "/example_configuration_object") {
    ///         return;
    ///     }
    ///     handle_update(json_patch_op);
    /// });
    ///
    /// je::add_hook({"/example_configuration_object/circumference_to_diameter_ratio"},
    ///               [](const std::string& json_path, const nlohmann::json& json_patch_op)
    /// {
    ///     // Update the "PI" global value in the static lua state.
    ///     if(json_patch_op.at("op")=="insert"||json_patch_op.at("op")=="replace"){
    ///         lua_pushnumber( Lua_state, json_patch_op.at("value").get<double>());
    ///         lua_setglobal( Lua_state, "PI");
    ///     }
    /// });
    /// je::run_hooks({{"path","/example_configuration_object/circumference_to_diameter_ratio"},
    ///                {"op","replace"}, {"value",4}});
    /// \endcode
    void add_hook(const auto& hook_triggers, hook&& hook)
    {
        using namespace irods::experimental::json_events::detail;
        auto& hooks = get_hooks();
        auto& edge_sets = get_edge_sets();
        size_t id = hooks.size();
        // Take ownership, we don't need to deal with any missing closures.
        // those can be nasty to debug.
        hooks.emplace_back(std::move(hook));
        for (const auto& name : hook_triggers) {
            edge_sets.insert(std::make_pair(name, id));
        }
    }

    /// \brief Run the hooks, the changes are a list of json patches.
    ///
    /// \param [in] changes The list of changes in json patch format, see
    /// RFC6902 for more details about that format.
    ///
    /// \code{.cpp}
    /// namespace je = irods::experimental::json_events;
    ///
    /// je::add_hook({je::KW_ALL_KEYS},
    ///              [](const std::string& json_path, const nlohmann::json& patch)
    /// {
    ///    std::cout << patch << std::endl;
    /// });
    /// // Simulate changing the delay server sleep interval
    /// je::run_hooks({{"path", "/advanced_settings/delay_server_sleep_time_in_seconds"},
    ///                {"op", "replace"},{"value", 12}});
    /// \endcode
    /// Change the delay_server_sleep_time_in_seconds in the configuration and send a
    /// SIGHUP signal to the toplevel irods process. The server should write
    /// something like
    /// `{ "path":"/advanced_settings/delay_server_sleep_time_in_seconds",
    ///    "op":"replace", "value":12 }` to stdout.
    void run_hooks(const nlohmann::json& changes);
} // namespace irods::experimental::json_events

#endif // IRODS_JSON_EVENTS_HPP
