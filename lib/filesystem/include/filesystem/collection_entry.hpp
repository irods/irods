#ifndef IRODS_FILESYSTEM_COLLECTION_ENTRY_HPP
#define IRODS_FILESYSTEM_COLLECTION_ENTRY_HPP

#include "filesystem/config.hpp"
#include "filesystem/filesystem.hpp"
#include "filesystem/path.hpp"
#include "filesystem/object_status.hpp"

#include <chrono>

namespace irods::experimental::filesystem
{
    namespace NAMESPACE_IMPL
    {
        class collection_iterator;
        class recursive_collection_iterator;
    } // namespace NAMESPACE_IMPL

    class collection_entry
    {
    public:
        // Observers

        // clang-format off
        operator const path&() const noexcept                     { return path_; }
        auto path() const noexcept -> const path&                 { return path_; }
        auto exists() const noexcept -> bool                      { return filesystem::NAMESPACE_IMPL::exists(status_); }
        auto is_data_object() const noexcept -> bool              { return filesystem::NAMESPACE_IMPL::is_data_object(status_); }
        auto is_collection() const noexcept -> bool               { return filesystem::NAMESPACE_IMPL::is_collection(status_); }
        auto is_other() const noexcept -> bool                    { return filesystem::NAMESPACE_IMPL::is_other(status_); }
        auto data_object_size() -> std::uintmax_t                 { return static_cast<std::uintmax_t>(data_size_); }
        auto creation_time() const noexcept -> object_time_type   { return ctime_; }
        auto last_write_time() const noexcept -> object_time_type { return mtime_; }
        auto status() const noexcept -> const object_status&      { return status_; }
        auto data_mode() const noexcept -> unsigned               { return data_mode_; }
        auto data_id() const noexcept -> const std::string&       { return data_id_; }
        auto checksum() const noexcept -> const std::string&      { return checksum_; }
        auto owner() const noexcept -> const std::string&         { return owner_; }
        auto data_type() const noexcept -> const std::string&     { return data_type_; }

        // Comparisons

        auto operator==(const collection_entry& _rhs) const noexcept -> bool { return path_ == _rhs.path_; }
        auto operator!=(const collection_entry& _rhs) const noexcept -> bool { return path_ != _rhs.path_; }
        auto operator< (const collection_entry& _rhs) const noexcept -> bool { return path_ <  _rhs.path_; }
        auto operator<=(const collection_entry& _rhs) const noexcept -> bool { return path_ <= _rhs.path_; }
        auto operator> (const collection_entry& _rhs) const noexcept -> bool { return path_ >  _rhs.path_; }
        auto operator>=(const collection_entry& _rhs) const noexcept -> bool { return path_ >= _rhs.path_; }
        // clang-format on

    private:
        friend class NAMESPACE_IMPL::collection_iterator;
        friend class NAMESPACE_IMPL::recursive_collection_iterator;

        mutable class path path_;
        mutable object_status status_;
        mutable unsigned data_mode_;
        mutable std::uintmax_t data_size_;
        mutable std::string data_id_;
        mutable object_time_type ctime_;
        mutable object_time_type mtime_;
        mutable std::string checksum_;
        mutable std::string owner_;
        mutable std::string data_type_;
    };
} // namespace irods::experimental::filesystem

#endif // IRODS_FILESYSTEM_COLLECTION_ENTRY_HPP
