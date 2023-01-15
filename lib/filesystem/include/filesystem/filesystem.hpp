#undef IRODS_FILESYSTEM_FILESYSTEM_HPP_INCLUDE_HEADER

#if defined(IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API)
    #if !defined(IRODS_FILESYSTEM_FILESYSTEM_HPP_FOR_SERVER)
        #define IRODS_FILESYSTEM_FILESYSTEM_HPP_FOR_SERVER
        #define IRODS_FILESYSTEM_FILESYSTEM_HPP_INCLUDE_HEADER
    #endif
#elif !defined(IRODS_FILESYSTEM_FILESYSTEM_HPP_FOR_CLIENT)
    #define IRODS_FILESYSTEM_FILESYSTEM_HPP_FOR_CLIENT
    #define IRODS_FILESYSTEM_FILESYSTEM_HPP_INCLUDE_HEADER
#endif

#ifdef IRODS_FILESYSTEM_FILESYSTEM_HPP_INCLUDE_HEADER

#include "filesystem/config.hpp"
#include "filesystem/object_status.hpp"
#include "filesystem/permissions.hpp"
#include "filesystem/copy_options.hpp"
#include "filesystem/filesystem_error.hpp"

#ifdef IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
    #include "rs_atomic_apply_metadata_operations.hpp"
#else
    #include "atomic_apply_metadata_operations.h"
#endif // IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API

#include "system_error.hpp"
#include "rodsErrorTable.h"

#include "json.hpp"

#include <cstdint>
#include <string>
#include <chrono>
#include <vector>
#include <type_traits>
#include <algorithm>

#include <boost/variant.hpp>

namespace irods::experimental::filesystem
{
    class path;

    using object_time_type = std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>;

#ifndef IRODS_FILESYSTEM_FILESYSTEM_COMMON_TYPES_AND_OBJECTS
#define IRODS_FILESYSTEM_FILESYSTEM_COMMON_TYPES_AND_OBJECTS
    //
    // The data types and objects defined in this space represent entities that
    // are shared across the client-side and server-side interfaces.
    //
    // These symbols MUST NOT be introduced into the space more than once!
    //

    enum class remove_options
    {
        none,
        no_trash
    };

    struct metadata
    {
        std::string attribute;
        std::string value;
        std::string units;
    };

    /// A tag type used to instruct an operation to run in administrator mode.
    const inline struct admin_tag {} admin;
#endif // IRODS_FILESYSTEM_FILESYSTEM_COMMON_TYPES_AND_OBJECTS

    namespace NAMESPACE_IMPL
    {
        // Operational functions

        auto copy(rxComm& _comm, const path& _from, const path& _to, copy_options _options = copy_options::none) -> void;
        auto copy_data_object(rxComm& _comm, const path& _from, const path& _to, copy_options _options = copy_options::none) -> bool;

        auto create_collection(rxComm& _comm, const path& _p) -> bool;
        auto create_collection(rxComm& _comm, const path& _p, const path& _existing_p) -> bool;
        auto create_collections(rxComm& _comm, const path& _p) -> bool;

        auto exists(const object_status& _s) noexcept -> bool;
        auto exists(rxComm& _comm, const path& _p) -> bool;

        /// \brief Checks if the path is registered in the catalog as a collection.
        ///
        /// \throws filesystem_error If the path is empty or exceeds the path limit.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _p    The path to a collection.
        ///
        /// \return A boolean indicating whether the collection is registered.
        auto is_collection_registered(rxComm& _comm, const path& _p) -> bool;

        /// \brief Checks if the path is registered in the catalog as a data object.
        ///
        /// \throws filesystem_error If the path is empty or exceeds the path limit.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _p    The path to a data object.
        ///
        /// \return A boolean indicating whether the data object is registered.
        auto is_data_object_registered(rxComm& _comm, const path& _p) -> bool;

        auto equivalent(rxComm& _comm, const path& _p1, const path& _p2) -> bool;

        /// \brief Returns the size of the latest good replica.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, or does not
        ///                          reference a data object.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _p    The path to a data object.
        ///
        /// \return An integer representing the size of the data object.
        auto data_object_size(rxComm& _comm, const path& _p) -> std::uintmax_t;

        auto is_collection(const object_status& _s) noexcept -> bool;
        auto is_collection(rxComm& _comm, const path& _p) -> bool;

        /// \brief Checks if the path points to a special collection.
        ///
        /// This function always inspects the collection entry in the catalog.
        ///
        /// \throws filesystem_error If the path is empty or exceeds the path limit.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _p    The path to a collection.
        ///
        /// \return A boolean indicating whether the collection is special.
        auto is_special_collection(rxComm& _comm, const path& _p) -> bool;

        auto is_empty(rxComm& _comm, const path& _p) -> bool;

        auto is_other(const object_status& _s) noexcept -> bool;
        auto is_other(rxComm& _comm, const path& _p) -> bool;

        auto is_data_object(const object_status& _s) noexcept -> bool;
        auto is_data_object(rxComm& _comm, const path& _p) -> bool;

        /// \brief Returns the mtime of the latest good replica or a collection.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, or does not
        ///                          reference a data object or collection.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _p    The path to a data object or collection.
        ///
        /// \return An object_time_type representing the mtime.
        auto last_write_time(rxComm& _comm, const path& _p) -> object_time_type;

        /// \brief Updates the mtime of a collection.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, or does not
        ///                          reference a collection.
        ///
        /// \param[in] _comm     The communication object.
        /// \param[in] _p        The path to a collection.
        /// \param[in] _new_time The new mtime.
        ///
        /// \return An object_time_type representing the mtime.
        auto last_write_time(rxComm& _comm, const path& _p, object_time_type _new_time) -> void;

        auto remove(rxComm& _comm, const path& _p, remove_options _opts = remove_options::none) -> bool;
        auto remove_all(rxComm& _comm, const path& _p, remove_options _opts = remove_options::none) -> std::uintmax_t;

        auto permissions(rxComm& _comm, const path& _p, const std::string& _user_or_group, perms _prms) -> void;

        /// \brief Modifies the permissions of a collection or data object.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, or does not
        ///                          reference a collection or data object.
        ///
        /// \param[in] _admin         A tag which instructs the function to operate in administrator mode.
        ///                           The client must be an administrator to use this function.
        /// \param[in] _comm          The communication object.
        /// \param[in] _p             The logical path to a collection or data object.
        /// \param[in] _user_or_group The user or group for which the permission will apply to.
        /// \param[in] _prms          The permission to set for \p _user_or_group.
        ///
        /// \since 4.2.11
        auto permissions(admin_tag, rxComm& _comm, const path& _p, const std::string& _user_or_group, perms _prms) -> void;

        /// \brief Modifies the inheritance option of a collection.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, or does not
        ///                          reference a collection.
        ///
        /// \param[in] _comm  The communication object.
        /// \param[in] _p     The path to a collection.
        /// \param[in] _value A boolean indicating whether inheritance should be enabled or not.
        ///
        /// \since 4.2.11
        auto enable_inheritance(rxComm& _comm, const path& _p, bool _value) -> void;

        /// \brief Modifies the inheritance option of a collection.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, or does not
        ///                          reference a collection.
        ///
        /// \param[in] _admin A tag which instructs the function to operate in administrator mode. The client
        ///                   must be an administrator to use this function.
        /// \param[in] _comm  The communication object.
        /// \param[in] _p     The path to a collection.
        /// \param[in] _value A boolean indicating whether inheritance should be enabled or not.
        ///
        /// \since 4.2.11
        auto enable_inheritance(admin_tag _admin, rxComm& _comm, const path& _p, bool _value) -> void;

        auto rename(rxComm& _comm, const path& _from, const path& _to) -> void;

        auto status(rxComm& _comm, const path& _p) -> object_status;

        auto status_known(const object_status& _s) noexcept -> bool;

        /// \brief Returns the checksum of the latest good replica.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, or does not
        ///                          reference a data object.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _p    The path to a data object.
        ///
        /// \return A string representing the checksum.
        auto data_object_checksum(rxComm& _comm, const path& _p) -> std::string;

        /// \brief Retrieves all metadata AVUs on a collection or data object.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, does not reference
        ///                          a collection or data object, or an error occurs.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _p    The path to a collection or data object.
        ///
        /// \return A list of metadata AVUs.
        auto get_metadata(rxComm& _comm, const path& _p) -> std::vector<metadata>;

        /// \brief Sets a single metadata AVU on a collection or data object.
        ///
        /// Upon successful execution, \p _p will have, at most, one metadata AVU that uses the attribute name
        /// defined by \p _metadata.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, does not reference
        ///                          a collection or data object, or an error occurs.
        ///
        /// \param[in] _comm     The communication object.
        /// \param[in] _p        The path to a collection or data object.
        /// \param[in] _metadata The metadata AVU to set.
        auto set_metadata(rxComm& _comm, const path& _p, const metadata& _metadata) -> void;

        /// \brief Sets a single metadata AVU on a collection or data object.
        ///
        /// Upon successful execution, \p _p will have, at most, one metadata AVU that uses the attribute name
        /// defined by \p _metadata.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, does not reference
        ///                          a collection or data object, or an error occurs.
        ///
        /// \param[in] _admin    A tag which instructs the function to operate in administrator mode. The client
        ///                      must be an administrator to use this function.
        /// \param[in] _comm     The communication object.
        /// \param[in] _p        The path to a collection or data object.
        /// \param[in] _metadata The metadata AVU to set.
        ///
        /// \since 4.2.12
        auto set_metadata(admin_tag _admin, rxComm& _comm, const path& _p, const metadata& _metadata) -> void;

        /// \brief Adds a single metadata AVU on a collection or data object.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, does not reference
        ///                          a collection or data object, or an error occurs.
        ///
        /// \param[in] _comm     The communication object.
        /// \param[in] _p        The path to a collection or data object.
        /// \param[in] _metadata The metadata AVU to add.
        auto add_metadata(rxComm& _comm, const path& _p, const metadata& _metadata) -> void;

        /// \brief Adds a single metadata AVU on a collection or data object.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, does not reference
        ///                          a collection or data object, or an error occurs.
        ///
        /// \param[in] _admin    A tag which instructs the function to operate in administrator mode. The client
        ///                      must be an administrator to use this function.
        /// \param[in] _comm     The communication object.
        /// \param[in] _p        The path to a collection or data object.
        /// \param[in] _metadata The metadata AVU to add.
        ///
        /// \since 4.2.12
        auto add_metadata(admin_tag _admin, rxComm& _comm, const path& _p, const metadata& _metadata) -> void;

        /// \brief Atomically adds multiple metadata AVUs to a collection or data object.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, does not reference
        ///                          a collection or data object, or an error occurs.
        ///
        /// \param[in] _comm  The communication object.
        /// \param[in] _p     The path to a collection or data object.
        /// \param[in] _first An iterator pointing to the first element of a metadata AVU range.
        /// \param[in] _last  An iterator pointing to the last element of a metadata AVU range (not inclusive).
        template <typename Iterator>
        auto add_metadata_atomic(rxComm& _comm, const path& _p, Iterator _first, Iterator _last) -> void;

        /// \brief Atomically adds multiple metadata AVUs to a collection or data object.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, does not reference
        ///                          a collection or data object, or an error occurs.
        ///
        /// \param[in] _admin A tag which instructs the function to operate in administrator mode. The client
        ///                   must be an administrator to use this function.
        /// \param[in] _comm  The communication object.
        /// \param[in] _p     The path to a collection or data object.
        /// \param[in] _first An iterator pointing to the first element of a metadata AVU range.
        /// \param[in] _last  An iterator pointing to the last element of a metadata AVU range (not inclusive).
        ///
        /// \since 4.2.12
        template <typename Iterator>
        auto add_metadata_atomic(admin_tag _admin, rxComm& _comm, const path& _p, Iterator _first, Iterator _last) -> void;

        /// \brief Atomically adds multiple metadata AVUs to a collection or data object.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, does not reference
        ///                          a collection or data object, or an error occurs.
        ///
        /// \param[in] _comm      The communication object.
        /// \param[in] _p         The path to a collection or data object.
        /// \param[in] _container The list of metadata AVUs to add.
        template <typename Container,
                  typename = decltype(std::begin(std::declval<Container>())),
                  typename = std::enable_if_t<std::is_same_v<std::decay_t<typename Container::value_type>, metadata>>>
        auto add_metadata_atomic(rxComm& _comm, const path& _p, const Container& _container) -> void;

        /// \brief Atomically adds multiple metadata AVUs to a collection or data object.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, does not reference
        ///                          a collection or data object, or an error occurs.
        ///
        /// \param[in] _admin     A tag which instructs the function to operate in administrator mode. The client
        ///                       must be an administrator to use this function.
        /// \param[in] _comm      The communication object.
        /// \param[in] _p         The path to a collection or data object.
        /// \param[in] _container The list of metadata AVUs to add.
        ///
        /// \since 4.2.12
        template <typename Container,
                  typename = decltype(std::begin(std::declval<Container>())),
                  typename = std::enable_if_t<std::is_same_v<std::decay_t<typename Container::value_type>, metadata>>>
        auto add_metadata_atomic(admin_tag _admin, rxComm& _comm, const path& _p, const Container& _container) -> void;

        /// \brief Removes a single metadata AVU from a collection or data object.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, does not reference
        ///                          a collection or data object, or an error occurs.
        ///
        /// \param[in] _comm     The communication object.
        /// \param[in] _p        The path to a collection or data object.
        /// \param[in] _metadata The metadata AVU to remove.
        auto remove_metadata(rxComm& _comm, const path& _p, const metadata& _metadata) -> void;

        /// \brief Removes a single metadata AVU from a collection or data object.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, does not reference
        ///                          a collection or data object, or an error occurs.
        ///
        /// \param[in] _admin    A tag which instructs the function to operate in administrator mode. The client
        ///                      must be an administrator to use this function.
        /// \param[in] _comm     The communication object.
        /// \param[in] _p        The path to a collection or data object.
        /// \param[in] _metadata The metadata AVU to remove.
        ///
        /// \since 4.2.12
        auto remove_metadata(admin_tag _admin, rxComm& _comm, const path& _p, const metadata& _metadata) -> void;

        /// \brief Atomically removes multiple metadata AVUs from a collection or data object.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, does not reference
        ///                          a collection or data object, or an error occurs.
        ///
        /// \param[in] _comm  The communication object.
        /// \param[in] _p     The path to a collection or data object.
        /// \param[in] _first An iterator pointing to the first element of a metadata AVU range.
        /// \param[in] _last  An iterator pointing to the last element of a metadata AVU range (not inclusive).
        template <typename Iterator>
        auto remove_metadata_atomic(rxComm& _comm, const path& _p, Iterator _first, Iterator _last) -> void;

        /// \brief Atomically removes multiple metadata AVUs from a collection or data object.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, does not reference
        ///                          a collection or data object, or an error occurs.
        ///
        /// \param[in] _admin A tag which instructs the function to operate in administrator mode. The client
        ///                   must be an administrator to use this function.
        /// \param[in] _comm  The communication object.
        /// \param[in] _p     The path to a collection or data object.
        /// \param[in] _first An iterator pointing to the first element of a metadata AVU range.
        /// \param[in] _last  An iterator pointing to the last element of a metadata AVU range (not inclusive).
        ///
        /// \since 4.2.12
        template <typename Iterator>
        auto remove_metadata_atomic(admin_tag _admin, rxComm& _comm, const path& _p, Iterator _first, Iterator _last) -> void;

        /// \brief Atomically removes multiple metadata AVUs from a collection or data object.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, does not reference
        ///                          a collection or data object, or an error occurs.
        ///
        /// \param[in] _comm      The communication object.
        /// \param[in] _p         The path to a collection or data object.
        /// \param[in] _container The list of metadata AVUs to remove.
        template <typename Container,
                  typename = decltype(std::begin(std::declval<Container>())),
                  typename = std::enable_if_t<std::is_same_v<std::decay_t<typename Container::value_type>, metadata>>>
        auto remove_metadata_atomic(rxComm& _comm, const path& _p, const Container& _container) -> void;

        /// \brief Atomically removes multiple metadata AVUs from a collection or data object.
        ///
        /// \throws filesystem_error If the path is empty, exceeds the path limit, does not reference
        ///                          a collection or data object, or an error occurs.
        ///
        /// \param[in] _admin     A tag which instructs the function to operate in administrator mode. The client
        ///                       must be an administrator to use this function.
        /// \param[in] _comm      The communication object.
        /// \param[in] _p         The path to a collection or data object.
        /// \param[in] _container The list of metadata AVUs to remove.
        ///
        /// \since 4.2.12
        template <typename Container,
                  typename = decltype(std::begin(std::declval<Container>())),
                  typename = std::enable_if_t<std::is_same_v<std::decay_t<typename Container::value_type>, metadata>>>
        auto remove_metadata_atomic(admin_tag _admin, rxComm& _comm, const path& _p, const Container& _container) -> void;

        #include "filesystem/filesystem.tpp"
    } // namespace NAMESPACE_IMPL
} // namespace irods::experimental::filesystem

#endif // IRODS_FILESYSTEM_FILESYSTEM_HPP_INCLUDE_HEADER
