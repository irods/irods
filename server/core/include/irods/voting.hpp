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

/// \brief Check to see whether \p _vote is equal to 0.
///
/// This function exists because comparing floating point values is fraught with peril.
///
/// \param[in] _vote The vote being compared to 0.
///
/// \retval true If \p _vote is considered equal to 0.
/// \retval false If \p _vote is not considered equal to 0.
///
/// \since 4.2.12
auto vote_is_zero(float _vote) -> bool;

} // namespace irods::experimental::resource::voting

#endif // VOTING_HPP
