#ifndef IRODS_ZONE_TYPE_HPP
#define IRODS_ZONE_TYPE_HPP

/// \file

#include <cstdint>

namespace irods::experimental::administration
{
    /// Defines the zone types.
    ///
    /// \since 4.2.8
    enum class zone_type : std::uint8_t
    {
        local, ///< Identifies the zone in which an operation originates.
        remote ///< Identifies a foreign zone.
    }; // enum class zone_type
} // namespace irods::experimental::administration

#endif // IRODS_ZONE_TYPE_HPP

