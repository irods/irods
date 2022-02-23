#ifndef __IRODS_GET_L1DESC_HPP__
#define __IRODS_GET_L1DESC_HPP__

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "irods/filesystem.hpp"

#include "irods/objDesc.hpp"

#include <optional>

namespace irods
{
    using l1_index_type = int;

    /// \returns Opened L1 descriptor specified opened L1 descriptor index
    auto get_l1desc(const l1_index_type _index) -> l1desc_t&;

    /// \brief Search for opened L1 descriptor based on logical path and hierarchy
    ///
    /// \param[in] _logical_path
    /// \param[in] _resource_hierarchy
    ///
    /// \return Pointer to l1desc_t
    /// \retval l1desc_t* If opened L1 descriptor with matching logical path and resource hierarchy is found
    /// \retval nulltpr If no matching opened L1 descriptor was found
    ///
    /// \since 4.2.9
    auto find_l1desc(
       const irods::experimental::filesystem::path& _logical_path,
       std::string_view _resource_hierarchy) -> l1desc_t*;
}; // namespace irods

#endif // __IRODS_GET_L1DESC_HPP__

