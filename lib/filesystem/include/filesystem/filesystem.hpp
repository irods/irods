#ifndef IRODS_FILESYSTEM_FILESYSTEM_HPP
#define IRODS_FILESYSTEM_FILESYSTEM_HPP

#include "filesystem/config.hpp"
#include "filesystem/object_status.hpp"
#include "filesystem/permissions.hpp"
#include "filesystem/copy_options.hpp"
#include "filesystem/filesystem_error.hpp"
#include "filesystem/detail.hpp"

#include "rcConnect.h"

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

    struct checksum
    {
        int replica_number;
        std::string value;
        std::uintmax_t size;
        bool is_up_to_date;
    };

    enum class replica_number
    {
        all
    };

    enum class verification_calculation
    {
        none,
        if_empty,
        always
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

        auto equivalent(rxComm& _comm, const path& _p1, const path& _p2) -> bool;

        auto data_object_size(rxComm& _comm, const path& _p) -> std::uintmax_t;

        auto is_collection(object_status _s) noexcept -> bool;
        auto is_collection(rxComm& _comm, const path& _p) -> bool;

        auto is_empty(rxComm& _comm, const path& _p) -> bool;

        auto is_other(object_status _s) noexcept -> bool;
        auto is_other(rxComm& _comm, const path& _p) -> bool;

        auto is_data_object(object_status _s) noexcept -> bool;
        auto is_data_object(rxComm& _comm, const path& _p) -> bool;

        auto last_write_time(rxComm& _comm, const path& _p) -> object_time_type;
        auto last_write_time(rxComm& _comm, const path& _p, object_time_type _new_time) -> void;

        auto remove(rxComm& _comm, const path& _p, remove_options _opts = remove_options::none) -> bool;
        auto remove(rxComm& _comm, const path& _p, extended_remove_options) -> bool;
        auto remove_all(rxComm& _comm, const path& _p, remove_options _opts = remove_options::none) -> std::uintmax_t;
        auto remove_all(rxComm& _comm, const path& _p, extended_remove_options _opts) -> std::uintmax_t;

        auto permissions(rxComm& _comm, const path& _p, const std::string& _user_or_group, perms _prms) -> void;

        auto rename(rxComm& _comm, const path& _from, const path& _to) -> void;

        auto status(rxComm& _comm, const path& _p) -> object_status;

        auto status_known(object_status _s) noexcept -> bool;

        auto data_object_checksum(rxComm& _comm,
                                  const path& _path,
                                  const std::variant<int, replica_number>& _replica_number,
                                  verification_calculation _calculation = verification_calculation::none) -> std::vector<checksum>;

        auto get_metadata(rxComm& _comm, const path& _path) -> std::vector<metadata>;

        auto set_metadata(rxComm& _comm, const path& _path, const metadata& _metadata) -> void;

        auto add_metadata(rxComm& _comm, const path& _path, const metadata& _metadata) -> void;

        template <typename Iterator>
        auto add_metadata(rxComm& _comm, const path& _path, Iterator _first, Iterator _last) -> void;

        template <typename Container,
                  typename = decltype(std::begin(std::declval<Container>())),
                  typename = std::enable_if_t<std::is_same_v<std::decay_t<typename Container::value_type>, metadata>>>
        auto add_metadata(rxComm& _comm, const path& _path, const Container& _container) -> void;

        auto remove_metadata(rxComm& _comm, const path& _path, const metadata& _metadata) -> void;

        template <typename Iterator>
        auto remove_metadata(rxComm& _comm, const path& _path, Iterator _first, Iterator _last) -> void;

        template <typename Container,
                  typename = decltype(std::begin(std::declval<Container>())),
                  typename = std::enable_if_t<std::is_same_v<std::decay_t<typename Container::value_type>, metadata>>>
        auto remove_metadata(rxComm& _comm, const path& _path, const Container& _container) -> void;

        #include "filesystem/filesystem.tpp"
    } // namespace NAMESPACE_IMPL
} // namespace irods::experimental::filesystem

#endif // IRODS_FILESYSTEM_FILESYSTEM_HPP
