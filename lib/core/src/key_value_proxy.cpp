#include "irods/key_value_proxy.hpp"

namespace irods::experimental
{
    auto make_key_value_proxy(std::initializer_list<key_value_pair> _kvps) -> key_value_proxy_pair<KeyValPair>
    {
        KeyValPair* cond_input = (KeyValPair*)malloc(sizeof(KeyValPair));
        std::memset(cond_input, 0, sizeof(KeyValPair));
        key_value_proxy proxy{*cond_input};
        for (const auto& kvp : _kvps) {
            const auto& key = std::get<0>(kvp);
            const auto& val = std::get<1>(kvp);
            proxy[key] = val;
        }
        return {std::move(proxy), lifetime_manager{*cond_input}};
    } // make_key_value_proxy

    auto keyword_has_a_value(const KeyValPair& _kvp, const std::string_view _keyword) -> bool
    {
        const auto cond_input = irods::experimental::make_key_value_proxy(_kvp);

        return cond_input.contains(_keyword) &&
               !cond_input.at(_keyword).value().empty();
    } // keyword_has_a_value
} // namespace irods::experimental
