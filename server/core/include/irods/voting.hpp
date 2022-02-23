#ifndef VOTING_HPP
#define VOTING_HPP

#include "irods/irods_plugin_context.hpp"
#include "irods/irods_resource_plugin.hpp"
#include "irods/irods_resource_redirect.hpp"

#include <string_view>

namespace irods::experimental::resource::voting {

namespace vote {
    constexpr float high{1.0};
    constexpr float medium{0.5};
    constexpr float low{0.25};
    constexpr float zero{0.0};
}

float calculate(
    std::string_view op,
    irods::plugin_context& ctx,
    std::string_view curr_host,
    const irods::hierarchy_parser& parser);

} // namespace irods::experimental::resource::voting

#endif // VOTING_HPP
