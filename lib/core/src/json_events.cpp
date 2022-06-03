#include "irods/json_events.hpp"

namespace irods::experimental::json_events
{
    const std::string KW_ALL_KEYS("ALL_KEYS");

    namespace detail
    {
        // This is where the function objects are stored, given that they will not be removed or reordered, their index
        // is what edge_sets' value refers to.
        std::vector<hook> hooks;
        // Edge sets, as in edge-triggered, contains an association between the changed property and an index into
        // hooks.
        std::unordered_multimap<std::string, std::size_t> edge_sets;

        std::vector<hook>& get_hooks()
        {
            return hooks;
        }

        std::unordered_multimap<std::string, size_t>& get_edge_sets()
        {
            return edge_sets;
        }
    } // namespace detail

    void run_hooks(const nlohmann::json& changes)
    {
        using namespace irods::experimental::json_events::detail;

        // The ALL_KEYS hook is triggered on every change in configuration value.
        auto all_key_hooks = edge_sets.equal_range(KW_ALL_KEYS);
        auto& hooks = get_hooks();
        auto& edge_sets = get_edge_sets();

        for (const auto& operation : changes) {
            const std::string& name = operation.at("path").get_ref<const std::string&>();

            for (auto it = all_key_hooks.first; it != all_key_hooks.second; ++it) {
                hooks[it->second](name, operation);
            }

            auto range = edge_sets.equal_range(name);
            for (auto it = range.first; it != range.second; ++it) {
                hooks[it->second](name, operation);
            }
        }
    }
} // namespace irods::experimental::json_events