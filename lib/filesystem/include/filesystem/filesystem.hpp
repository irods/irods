#ifndef IRODS_FILESYSTEM_FILESYSTEM_HPP
#define IRODS_FILESYSTEM_FILESYSTEM_HPP

#include "filesystem/config.hpp"
#include "filesystem/object_status.hpp"
#include "filesystem/permissions.hpp"
#include "filesystem/copy_options.hpp"
#include "filesystem/filesystem_error.hpp"
#include "filesystem/detail.hpp"

#ifdef IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
    #include "rs_atomic_apply_metadata_operations.hpp"
#else
    #include "atomic_apply_metadata_operations.h"
#endif // IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API

#include "json.hpp"

#include <cstdint>
#include <string>
#include <chrono>
#include <vector>
#include <variant>
#include <type_traits>
#include <algorithm>

namespace irods::experimental::filesystem
{
    class path;

    using object_time_type = std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>;

    enum class remove_options
    {
        none,
        no_trash
    };

    struct extended_remove_options
    {
        bool no_trash   = false;
        bool verbose    = false;
        bool progress   = false;
        bool recursive  = false;
        bool unregister = false;
    };

    struct metadata
    {
        std::string attribute;
        std::string value;
        std::string units;
    };

    namespace NAMESPACE_IMPL
    {
        // Operational functions

        auto copy(rxComm& _comm, const path& _from, const path& _to, copy_options _options = copy_options::none) -> void;
        auto copy_data_object(rxComm& _comm, const path& _from, const path& _to, copy_options _options = copy_options::none) -> bool;

        auto create_collection(rxComm& _comm, const path& _p) -> bool;
        auto create_collection(rxComm& _comm, const path& _p, const path& _existing_p) -> bool;
        auto create_collections(rxComm& _comm, const path& _p) -> bool;

        auto exists(object_status _s) noexcept -> bool;
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

        auto is_collection(object_status _s) noexcept -> bool;
        auto is_collection(rxComm& _comm, const path& _p) -> bool;

        auto is_empty(rxComm& _comm, const path& _p) -> bool;

        auto is_other(object_status _s) noexcept -> bool;
        auto is_other(rxComm& _comm, const path& _p) -> bool;

        auto is_data_object(object_status _s) noexcept -> bool;
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
        auto remove(rxComm& _comm, const path& _p, extended_remove_options _opts) -> bool;
        auto remove_all(rxComm& _comm, const path& _p, remove_options _opts = remove_options::none) -> std::uintmax_t;
        auto remove_all(rxComm& _comm, const path& _p, extended_remove_options _opts) -> std::uintmax_t;

        auto permissions(rxComm& _comm, const path& _p, const std::string& _user_or_group, perms _prms) -> void;

        auto rename(rxComm& _comm, const path& _from, const path& _to) -> void;

        auto status(rxComm& _comm, const path& _p) -> object_status;

        auto status_known(object_status _s) noexcept -> bool;

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

        auto get_metadata(rxComm& _comm, const path& _p) -> std::vector<metadata>;

        auto set_metadata(rxComm& _comm, const path& _p, const metadata& _metadata) -> void;

        auto add_metadata(rxComm& _comm, const path& _p, const metadata& _metadata) -> void;

        template <typename Iterator>
        auto add_metadata(rxComm& _comm, const path& _p, Iterator _first, Iterator _last) -> void;

        template <typename Container,
                  typename = decltype(std::begin(std::declval<Container>())),
                  typename = std::enable_if_t<std::is_same_v<std::decay_t<typename Container::value_type>, metadata>>>
        auto add_metadata(rxComm& _comm, const path& _p, const Container& _container) -> void;

        auto remove_metadata(rxComm& _comm, const path& _p, const metadata& _metadata) -> void;

        template <typename Iterator>
        auto remove_metadata(rxComm& _comm, const path& _p, Iterator _first, Iterator _last) -> void;

        template <typename Container,
                  typename = decltype(std::begin(std::declval<Container>())),
                  typename = std::enable_if_t<std::is_same_v<std::decay_t<typename Container::value_type>, metadata>>>
        auto remove_metadata(rxComm& _comm, const path& _p, const Container& _container) -> void;

        #include "filesystem/filesystem.tpp"
    } // namespace NAMESPACE_IMPL
} // namespace irods::experimental::filesystem

#endif // IRODS_FILESYSTEM_FILESYSTEM_HPP
