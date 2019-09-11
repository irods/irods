#ifndef IRODS_METADATA_HPP
#define IRODS_METADATA_HPP

#include <string>

namespace irods::experimental
{
    struct metadata
    {
        std::string name;
        std::string value;
        std::string units;
    };
} // namespace irods::experimental

#endif // IRODS_METADATA_HPP

