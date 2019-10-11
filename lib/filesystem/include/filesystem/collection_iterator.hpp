#ifndef IRODS_FILESYSTEM_COLLECTION_ITERATOR_HPP
#define IRODS_FILESYSTEM_COLLECTION_ITERATOR_HPP

#include "filesystem/config.hpp"
#include "filesystem/filesystem.hpp"
#include "filesystem/collection_entry.hpp"
#include "filesystem/path.hpp"

#include "rcConnect.h"

#include <iterator>
#include <memory>

namespace irods::experimental::filesystem::NAMESPACE_IMPL
{
    enum class collection_options
    {
        none,
        skip_permission_denied
    };

    class collection_iterator
    {
    public:
        // clang-format off
        using value_type        = collection_entry;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const value_type*;
        using reference         = const value_type&;
        using iterator_category = std::input_iterator_tag;
        // clang-format on

        // Constructors and destructor

        collection_iterator() = default;

        collection_iterator(rxComm& _comm,
                            const path& _p,
                            collection_options _opts = collection_options::none);

        collection_iterator(const collection_iterator& _other) = default;
        auto operator=(const collection_iterator& _other) -> collection_iterator& = default;

        collection_iterator(collection_iterator&& _other) = default;
        auto operator=(collection_iterator&& _other) -> collection_iterator& = default;

        ~collection_iterator();

        // Observers

        auto connection() -> rxComm* { return ctx_->comm; }

        // clang-format off
        auto operator*() const -> reference { return ctx_->entry; }
        auto operator->() const -> pointer  { return &ctx_->entry; }
        // clang-format on

        // Modifiers

        auto operator++() -> collection_iterator&;

        // Compare

        // clang-format off
        auto operator==(const collection_iterator& _rhs) const noexcept -> bool { return _rhs.ctx_ == ctx_; }
        auto operator!=(const collection_iterator& _rhs) const noexcept -> bool { return !(*this == _rhs); }
        // clang-format on

    private:
        struct context
        {
            rxComm* comm{};
            path path{};
            int handle{};
            value_type entry{};
        };

        std::shared_ptr<context> ctx_;
    };

    // Enables support for range-based for-loops.

    inline auto begin(collection_iterator _iter) noexcept -> collection_iterator
    {
        return _iter;
    }

    inline auto end(const collection_iterator&) noexcept -> const collection_iterator
    {
        return {};
    }
} // namespace irods::experimental::filesystem::NAMESPACE_IMPL

#endif // IRODS_FILESYSTEM_COLLECTION_ITERATOR_HPP
