#include "filesystem/recursive_collection_iterator.hpp"

#include "filesystem/filesystem_error.hpp"

#include <iostream>

namespace irods {
namespace experimental {
namespace filesystem {
namespace NAMESPACE_IMPL {

    // Constructors and destructor

    recursive_collection_iterator::recursive_collection_iterator(rxComm& _comm,
                                                                 const path& _p,
                                                                 collection_options _opts)
        : ctx_{std::make_shared<context>()}
    {
        ctx_->stack.emplace(_comm, _p, _opts);

        if (ctx_->stack.empty()) {
            throw filesystem_error{"construction error"};
        }
    }

    // Modifiers

    auto recursive_collection_iterator::operator++() -> recursive_collection_iterator&
    {
        auto& iter = ctx_->stack.top();
        bool added_new_collection = false;

        if (iter->is_collection() && ctx_->recurse) {
            // Add the collection to the stack only if it is not empty.
            if (!is_empty(*iter.connection(), iter->path()))
            {
                added_new_collection = true;
                auto* conn = iter.connection();
                auto path = std::move(iter->path());

                // Move the iterator forward to avoid re-entering the same
                // collection when the stack is popped.
                if (collection_iterator{} == ++iter) {
                    ctx_->stack.pop();
                }

                ctx_->stack.emplace(*conn, path, ctx_->opts);
            }
        }

        // Handle leaving a collection. Increment the iterator only if a new
        // collection was NOT pushed on to the stack. Reordering the conditions
        // in this if-stmt will make this iterator impl produce incorrect results.
        if (!added_new_collection && collection_iterator{} == ++iter) {
            ctx_->stack.pop();

            if (ctx_->stack.empty()) {
                return *this = {};
            }
        }

        // Iterating forward must always reset the iterator to recurse into
        // subcollections as defined by the C++ Std.
        ctx_->recurse = true;

        return *this;
    }

    auto recursive_collection_iterator::pop() -> void
    {
        // If there is only a single iterator on the stack, then popping it must
        // produce the end iterator.
        if (ctx_->stack.size() == 1) {
            *this = {};
            return;
        }

        ctx_->stack.pop();
    }

} // namespace NAMESPACE_IMPL
} // namespace filesystem
} // namespace experimental
} // namespace irods

