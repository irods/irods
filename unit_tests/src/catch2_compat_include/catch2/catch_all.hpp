#ifndef IRODS_CATCH2_V2_SHIM_HPP
#define IRODS_CATCH2_V2_SHIM_HPP

#include <catch2/catch.hpp>

// NOLINTBEGIN(misc-unused-alias-decls)

namespace Catch
{
    namespace Matchers
    {
        template <typename... Args>
        constexpr auto ContainsSubstring(Args&&... args) -> decltype(Contains(std::forward<Args>(args)...))
        {
            return Contains(std::forward<Args>(args)...);
        }

        using Impl::MatcherBase;
    } //namespace Matchers

    namespace clara { }
    namespace Clara = clara;
} //namespace Catch

// NOLINTEND(misc-unused-alias-decls)

#endif // IRODS_CATCH2_V2_SHIM_HPP
