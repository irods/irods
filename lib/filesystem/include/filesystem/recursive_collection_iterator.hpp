#ifndef IRODS_FILESYSTEM_RECURSIVE_COLLECTION_ITERATOR_HPP
#define IRODS_FILESYSTEM_RECURSIVE_COLLECTION_ITERATOR_HPP

#include "filesystem/config.hpp"
#include "filesystem/collection_iterator.hpp"
#include "filesystem/collection_entry.hpp"

#include <iterator>
#include <stack>
#include <memory>

namespace irods::experimental::filesystem::NAMESPACE_IMPL
{
    class recursive_collection_iterator
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

        recursive_collection_iterator() = default;

        recursive_collection_iterator(rxComm& _comm,
                                      const path& _p,
                                      collection_options _opts = collection_options::none);

        recursive_collection_iterator(const recursive_collection_iterator& _other) = default;
        auto operator=(const recursive_collection_iterator& _other) -> recursive_collection_iterator& = default;

        recursive_collection_iterator(recursive_collection_iterator&& _other) = default;
        auto operator=(recursive_collection_iterator&& _other) -> recursive_collection_iterator& = default;

        ~recursive_collection_iterator() = default;

        // Observers

        auto connection() -> rxComm* { return ctx_->stack.empty() ? nullptr : (*this).connection(); }

        // clang-format off
        auto operator*() const -> reference { return *ctx_->stack.top(); }
        auto operator->() const -> pointer  { return &*ctx_->stack.top(); }

        auto options() const noexcept -> collection_options { return ctx_->opts; }
        auto depth() const noexcept -> int                  { return static_cast<int>(ctx_->stack.size()) - 1; }
        auto recursion_pending() const noexcept -> bool     { return ctx_->recurse; }
        // clang-format on

        // Modifiers

        auto operator++() -> recursive_collection_iterator&;
        
        auto pop() -> void;
        auto disable_recursion_pending() noexcept -> void { ctx_->recurse = false; }

        // Compare

        // clang-format off
        auto operator==(const recursive_collection_iterator& _rhs) const noexcept -> bool { return _rhs.ctx_ == ctx_; }
        auto operator!=(const recursive_collection_iterator& _rhs) const noexcept -> bool { return !(*this == _rhs); }
        // clang-format on

    private:
        struct context
        {
            std::stack<collection_iterator> stack;
            collection_options opts = collection_options::none;
            bool recurse = true;
        };

        std::shared_ptr<context> ctx_;
    };

    // Enables support for range-based for-loops.

    inline auto begin(recursive_collection_iterator _iter) noexcept -> recursive_collection_iterator
    {
        return _iter;
    }

    inline auto end(const recursive_collection_iterator&) noexcept -> const recursive_collection_iterator
    {
        return {};
    }
} // namespace irods::experimental::filesystem::NAMESPACE_IMPL

#endif // IRODS_FILESYSTEM_RECURSIVE_COLLECTION_ITERATOR_HPP
