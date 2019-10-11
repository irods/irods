#include "filesystem/collection_iterator.hpp"

#include "filesystem/detail.hpp"
#include "filesystem/filesystem_error.hpp"

// clang-format off
#ifdef IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
    #include "rsOpenCollection.hpp"
    #include "rsReadCollection.hpp"
    #include "rsCloseCollection.hpp"
#else
    #include "openCollection.h"
    #include "readCollection.h"
    #include "closeCollection.h"
#endif // IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
// clang-format on

#include "irods_at_scope_exit.hpp"

#include <functional>
#include <string>
#include <cassert>

namespace irods::experimental::filesystem::NAMESPACE_IMPL
{
#ifdef IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
    namespace
    {
        const auto rsReadCollection = [](rsComm_t* _comm, int _handle, collEnt_t** _collEnt) -> int
        {
            return ::rsReadCollection(_comm, &_handle, _collEnt);
        };

        const auto rsCloseCollection = [](rsComm_t* _comm, int _handle) -> int
        {
            return ::rsCloseCollection(_comm, &_handle);
        };
    } // anonymous namespace
#endif // IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API

    collection_iterator::collection_iterator(rxComm& _comm,
                                             const path& _p,
                                             collection_options _opts)
        : ctx_{}
    {
        detail::throw_if_path_length_exceeds_limit(_p);
    
        ctx_ = std::make_shared<context>();
        ctx_->comm = &_comm;
        ctx_->path = _p;
        assert(ctx_->handle == 0);

        collInp_t input{};
        std::strncpy(input.collName, _p.c_str(), _p.string().size());

        ctx_->handle = rxOpenCollection(&_comm, &input);

        if (ctx_->handle < 0) {
            throw filesystem_error{"could not open collection for reading [handle => " +
                                   std::to_string(ctx_->handle) + ']'};
        }

        // Point to the first entry.
        ++(*this);
    }

    collection_iterator::~collection_iterator()
    {
        if (ctx_.use_count() == 1) {
            rxCloseCollection(ctx_->comm, ctx_->handle);
        }
    }

    auto collection_iterator::operator++() -> collection_iterator&
    {
        collEnt_t* e{};

        if (const auto ec = rxReadCollection(ctx_->comm, ctx_->handle, &e); ec < 0) {
            if (ec == CAT_NO_ROWS_FOUND) {
                ctx_ = nullptr;
                return *this;
            }
            
            throw filesystem_error{"could not read collection entry [error code => " +
                                   std::to_string(ec) + ']'};
        }

        irods::at_scope_exit<std::function<void()>> at_scope_exit{[e] {
            if (e) {
#ifdef IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
                std::free(e);
#else
                freeCollEnt(e);
#endif
            }
        }};

        auto& entry = ctx_->entry;

        entry.data_mode_ = e->dataMode;
        entry.data_size_ = static_cast<std::uintmax_t>(e->dataSize);

        // clang-format off
        if (e->dataId)      { entry.data_id_ = e->dataId; }
        if (e->createTime)  { entry.ctime_ = object_time_type{std::chrono::seconds{std::stoll(e->createTime)}}; }
        if (e->modifyTime)  { entry.mtime_ = object_time_type{std::chrono::seconds{std::stoll(e->modifyTime)}}; }
        if (e->chksum)      { entry.checksum_ = e->chksum; }
        if (e->ownerName)   { entry.owner_ = e->ownerName; }
        if (e->dataType)    { entry.data_type_ = e->dataType; }
        // clang-format on

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
} // namespace irods::experimental::filesystem::NAMESPACE_IMPL

