#ifndef IRODS_FILESYSTEM_FILESYSTEM_HPP
#define IRODS_FILESYSTEM_FILESYSTEM_HPP

#include "filesystem/config.hpp"
#include "filesystem/object_status.hpp"
#include "filesystem/permissions.hpp"
#include "filesystem/copy_options.hpp"

#include "rcConnect.h"

#include <cstdint>
#include <string>
#include <istream>
#include <ostream>
#include <chrono>
#include <optional>
#include <vector>
#include <variant>

namespace irods::experimental::filesystem
{
    class path;

    using object_time_type = std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>;

    enum class remove_options
    {
        none,
        no_trash
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
        std::optional<std::string> units;
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
        auto remove_all(rxComm& _comm, const path& _p, remove_options _opts = remove_options::none) -> std::uintmax_t;

        auto permissions(rxComm& _comm, const path& _p, perms _prms) -> void;

        auto rename(rxComm& _comm, const path& _from, const path& _to) -> void;

        auto status(rxComm& _comm, const path& _p) -> object_status;

        auto status_known(object_status _s) noexcept -> bool;

        auto data_object_checksum(rxComm& _comm,
                                  const path& _path,
                                  const std::variant<int, replica_number>& _replica_number,
                                  verification_calculation _calculation = verification_calculation::none) -> std::vector<checksum>;

        auto get_metadata(rxComm& _comm,
                          const path& _path,
                          const std::optional<metadata>& _metadata = {}) -> std::vector<metadata>;

        auto set_metadata(rxComm& _comm, const path& _path, const metadata& _metadata) -> bool;

        auto remove_metadata(rxComm& _comm, const path& _path, const metadata& _metadata) -> bool;
    } // namespace NAMESPACE_IMPL
} // namespace irods::experimental::filesystem

#endif // IRODS_FILESYSTEM_FILESYSTEM_HPP
