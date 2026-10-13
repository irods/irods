#include "irods/filesystem/collection_iterator.hpp"

#include "irods/filesystem/path_utilities.hpp"
#include "irods/filesystem/filesystem_error.hpp"

#ifdef IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#  include "irods/rsOpenCollection.hpp"
#  include "irods/rsReadCollection.hpp"
#  include "irods/rsCloseCollection.hpp"
#endif // IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API

#include "irods/irods_at_scope_exit.hpp"
#include "irods/system_error.hpp"

#include <cassert>
#include <charconv>
#include <chrono>
#include <functional>
#include <string>
#include <string_view>
#include <system_error>

namespace irods::experimental::filesystem::NAMESPACE_IMPL
{
    collection_iterator::collection_iterator(rxComm& _comm,
                                             const path& _p,
                                             collection_options _opts)
        : ctx_{}
    {
        throw_if_path_length_exceeds_limit(_p);

        ctx_ = std::make_shared<context>();
        ctx_->comm = &_comm;
        ctx_->path = _p;

#ifdef IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
        assert(ctx_->handle == 0);

        collInp_t input{};
        std::strncpy(input.collName, _p.c_str(), _p.string().size());

        ctx_->handle = rsOpenCollection(&_comm, &input);

        if (ctx_->handle < 0) {
            throw filesystem_error{"could not open collection for reading", make_error_code(ctx_->handle)};
        }
#else
        const auto no_flags = 0;

        if (const auto ec = rclOpenCollection(&_comm, const_cast<char*>(_p.c_str()), no_flags, &ctx_->handle); ec < 0) {
            throw filesystem_error{"could not open collection for reading", make_error_code(ec)};
        }
#endif // IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API

        // Point to the first entry.
        ++(*this);
    }

    collection_iterator::~collection_iterator()
    {
        close();
    }

    auto collection_iterator::operator++() -> collection_iterator&
    {
        collEnt_t* e{};

#ifdef IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
        const auto ec = rsReadCollection(ctx_->comm, &ctx_->handle, &e);
#else
        collEnt_t ce{};
        e = &ce;
        const auto ec = rclReadCollection(ctx_->comm, &ctx_->handle, &ce);
#endif // IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API

        if (ec < 0) {
            if (ec == CAT_NO_ROWS_FOUND) {
                close();
                ctx_ = nullptr;
                return *this;
            }

            throw filesystem_error{"could not read collection entry", make_error_code(ec)};
        }

#ifdef IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
        irods::at_scope_exit free_collection_entry{[&e] { 
            if (e) {
                // Do NOT free the contents of the collEnt_t. Its contents is managed
                // by the rs* collection APIs.
                std::free(e);
            }
        }};
#else
        // The rcl* collection functions manage memory for us. These functions
        // will free any heap allocated memory for us. Attempting to free any memory
        // returned from rclReadCollection will result in a crash.
#endif

        auto& entry = ctx_->entry;

        entry.data_mode_ = e->dataMode;
        entry.data_size_ = static_cast<std::uintmax_t>(e->dataSize);

        // clang-format off
        if (e->dataId)     { entry.data_id_ = e->dataId; }
        if (e->chksum)     { entry.checksum_ = e->chksum; }
        if (e->ownerName)  { entry.owner_ = e->ownerName; }
        if (e->dataType)   { entry.data_type_ = e->dataType; }
        // clang-format on
        if (e->createTime) {
            std::chrono::seconds::rep ctime; // NOLINT(cppcoreguidelines-init-variables)
            const std::string_view ctime_str{e->createTime};
            const auto* ctime_b = ctime_str.data();
            const auto* ctime_e = ctime_b + ctime_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto [ptr, ec] = std::from_chars(ctime_b, ctime_e, ctime);
            if (std::errc{} != ec) {
                throw filesystem_error{"could not parse create time", std::make_error_code(ec)};
            }
            entry.ctime_ = object_time_type{std::chrono::seconds{ctime}};
        }
        if (e->modifyTime) {
            std::chrono::seconds::rep mtime; // NOLINT(cppcoreguidelines-init-variables)
            const std::string_view mtime_str{e->modifyTime};
            const auto* mtime_b = mtime_str.data();
            const auto* mtime_e = mtime_b + mtime_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto [ptr, ec] = std::from_chars(mtime_b, mtime_e, mtime);
            if (std::errc{} != ec) {
                throw filesystem_error{"could not parse modify time", std::make_error_code(ec)};
            }
            entry.mtime_ = object_time_type{std::chrono::seconds{mtime}};
        }

        switch (e->objType) {
            case COLL_OBJ_T:
                entry.status_.type(object_type::collection);
                entry.path_ = e->collName;
                break;

            case DATA_OBJ_T:
                entry.status_.type(object_type::data_object);
                entry.path_ = ctx_->path / e->dataName;
                break;

            default:
                entry.status_.type(object_type::none);
                break;
        }

        return *this;
    }

    auto collection_iterator::close() -> void
    {
        if (ctx_.use_count() == 1) {
#ifdef IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
            rsCloseCollection(ctx_->comm, &ctx_->handle);
#else
            rclCloseCollection(&ctx_->handle);
#endif // IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
        }
    }
} // namespace irods::experimental::filesystem::NAMESPACE_IMPL
