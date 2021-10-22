#ifndef IRODS_FIXED_BUFFER_RESOURCE_HPP
#define IRODS_FIXED_BUFFER_RESOURCE_HPP

/// \file

#include <boost/container/pmr/memory_resource.hpp>
#include <boost/container/pmr/vector.hpp>
#include <boost/container/pmr/string.hpp>

#include <fmt/format.h>

#include <cstddef>
#include <cassert>
#include <iterator>
#include <algorithm>
#include <type_traits>
#include <ostream>
#include <stdexcept>

/// A namespace containing components meant to be used with Boost.Container's PMR library.
namespace irods::experimental::pmr
{
    /// A \p fixed_buffer_resource is a special purpose memory resource class template that
    /// allocates memory from the buffer given on construction. It allows applications to
    /// enforce a cap on the amount of memory available to components.
    ///
    /// This class implements a first-fit scheme and is NOT thread-safe.
    ///
    /// \tparam ByteRep The memory representation for the underlying buffer. Must be \p char
    ///                 or \p std::byte.
    ///
    /// \since 4.2.11
    template <typename ByteRep>
    class fixed_buffer_resource
        : public boost::container::pmr::memory_resource
    {
    public:
        static_assert(std::is_same_v<ByteRep, char> || std::is_same_v<ByteRep, std::byte>);

        /// Constructs a \p fixed_buffer_resource using the given buffer as the allocation
        /// source.
        ///
        /// \param[in] _buffer                The buffer that will be used for allocations.
        /// \param[in] _buffer_size           The size of the buffer in bytes.
        /// \param[in] _initial_freelist_size The initial size of the freelist in terms of elements.
        ///
        /// \throws std::invalid_argument If any of the incoming constructor arguments do not
        ///                               satisfy construction requirements.
        ///
        /// \since 4.2.11
        fixed_buffer_resource(ByteRep* _buffer,
                              std::int64_t _buffer_size,
                              std::int64_t _initial_freelist_size)
            : boost::container::pmr::memory_resource{}
            , buffer_{_buffer}
            , buffer_size_(_buffer_size)
            , allocated_{}
            , overhead_{management_block_size}
            , headers_{}
            , freelist_{}
        {
            if (!_buffer || _buffer_size <= 0 || _initial_freelist_size <= 0) {
                const auto* msg_fmt = "fixed_buffer_resource: invalid constructor arguments "
                                      "[buffer={}, size={}, freelist_size={}].";
                const auto msg = fmt::format(msg_fmt, static_cast<void*>(_buffer), _buffer_size, _initial_freelist_size);
                throw std::invalid_argument{msg};
            }

            headers_ = reinterpret_cast<header*>(buffer_);
            headers_->size = buffer_size_ - sizeof(header);
            headers_->prev = nullptr;
            headers_->next = nullptr;
            *address_of_used_flag(headers_) = false;

            freelist_.reserve(_initial_freelist_size);
            freelist_.push_back(headers_);
        } // fixed_buffer_resource

        fixed_buffer_resource(const fixed_buffer_resource&) = delete;
        auto operator=(const fixed_buffer_resource&) -> fixed_buffer_resource& = delete;

        ~fixed_buffer_resource() = default;

        /// Returns the number of bytes used by the client.
        ///
        /// The value returned does not include memory used for tracking allocations.
        ///
        /// \return An unsigned integral type.
        ///
        /// \since 4.2.11
        auto allocated() const noexcept -> std::size_t
        {
            return allocated_;
        } // allocated

        /// Returns the number of bytes used for tracking allocations.
        ///
        /// \return An unsigned integral type.
        ///
        /// \since 4.2.11
        auto allocation_overhead() const noexcept -> std::size_t
        {
            return overhead_;
        } // allocation_overhead

        /// Writes the state of the allocation table to the output stream.
        ///
        /// \since 4.2.11
        auto print(std::ostream& _os) const -> void
        {
            std::size_t i = 0;

            for (auto* h = headers_; h; h = h->next) {
                _os << fmt::format("{:>3}. Header Info [{}]: {{previous={:14}, next={:14}, used={}, data_size={}, data={:14}}}\n",
                                   i,
                                   fmt::ptr(h),
                                   fmt::ptr(h->prev),
                                   fmt::ptr(h->next),
                                   is_data_in_use(h),
                                   h->size,
                                   fmt::ptr(address_of_data_segment(h)));
                ++i;
            }
        } // print

    protected:
        auto do_allocate(std::size_t _bytes, std::size_t _alignment) -> void* override
        {
            for (auto* h : freelist_) {
                if (auto* p = allocate_block(_bytes, _alignment, h); p) {
                    remove_header_from_freelist(h);
                    return p;
                }
            }

            throw std::bad_alloc{};
        } // do_allocate

        auto do_deallocate(void* _p, std::size_t _bytes, std::size_t _alignment) -> void override
        {
            auto* h = reinterpret_cast<header*>(static_cast<ByteRep*>(_p) - management_block_size);
            auto* used = address_of_used_flag(h);

            assert(*used);
            assert(h->size == _bytes);

            *used = false;

            freelist_.push_back(h);

            coalesce_with_next_unused_block(h);
            coalesce_with_next_unused_block(h->prev);

            allocated_ -= _bytes;
        } // do_deallocate

        auto do_is_equal(const boost::container::pmr::memory_resource& _other) const noexcept -> bool override
        {
            return this == &_other;
        } // do_is_equal

    private:
        // Metadata information associated with a single data segment within the
        // underlying memory buffer. All data segments will be preceded by a header.
        struct header
        {
            std::size_t size;   // Size of the memory block (excluding header).
            header* prev;       // Pointer to the previous header block.
            header* next;       // Pointer to the next header block.
        }; // struct header

        // The size of the header just before the data segment.
        // represents the "used" flag in the metadata associated with the data segment.
        static constexpr std::size_t management_block_size = sizeof(header) + sizeof(bool);

        auto address_of_used_flag(header* _h) const noexcept -> bool*
        {
            return static_cast<bool*>(static_cast<void*>(reinterpret_cast<ByteRep*>(_h) + sizeof(header)));
        } // address_of_used_flag

        auto address_of_data_segment(header* _h) const noexcept -> ByteRep*
        {
            return reinterpret_cast<ByteRep*>(_h) + management_block_size;
        } // address_of_data_segment

        auto is_data_in_use(header* _h) const noexcept -> bool
        {
            return *address_of_used_flag(_h);
        } // is_data_in_use

        auto allocate_block(std::size_t _bytes, std::size_t _alignment, header* _h) -> void*
        {
            if (is_data_in_use(_h)) {
                return nullptr;
            }

            // Return the block if it matches the requested number of bytes.
            if (_bytes == _h->size) {
                *address_of_used_flag(_h) = true;
                allocated_ += _bytes;
                return address_of_data_segment(_h);
            }

            // Split the data segment managed by this header if it is large enough
            // to satisfy the allocation request and a new header block.
            if (management_block_size + _bytes < _h->size) {
                // The unused memory is located right after the header.
                auto* data = address_of_data_segment(_h);

#if 0
                //
                // This disabled block of code exists here as a reminder that this allocator
                // implementation does not handle memory alignment requests. More understanding
                // is needed before this can be addressed.
                //

                void* vdata = data;
                auto* aligned_data = std::align(_alignment, _bytes, vdata, _h->size);
                
                if (!aligned_data) {
                    continue;
                }
#endif

                // Insert a new header after "_h"'s data segment.
                // The new header manages unused memory.
                auto* new_header = reinterpret_cast<header*>(data + _bytes);
                new_header->size = _h->size - _bytes - management_block_size;
                new_header->prev = _h;
                new_header->next = _h->next;
                *address_of_used_flag(new_header) = false;

                freelist_.push_back(new_header);

                // Update the allocation table links for the header just after the
                // newly added header.
                if (auto* next_header = _h->next; next_header) {
                    next_header->prev = new_header;
                }

                // Adjust the current header's size and mark it as used.
                _h->size = _bytes;
                _h->next = new_header;
                *address_of_used_flag(_h) = true;

                allocated_ += _bytes;
                overhead_ += management_block_size;

                return data;
            }

            return nullptr;
        } // allocate_block

        auto coalesce_with_next_unused_block(header* _h) -> void
        {
            if (!_h || is_data_in_use(_h)) {
                return;
            }

            auto* header_to_remove = _h->next;

            // Coalesce the memory blocks if they are not in use by the client.
            // This means that "_h" will absorb the header at "_h->next".
            if (header_to_remove && !is_data_in_use(header_to_remove)) {
                _h->size += header_to_remove->size;
                _h->next = header_to_remove->next;

                // Make sure the links between the headers are updated appropriately.
                // (i.e. "_h" and "header_to_remove->next" need their links updated).
                if (auto* new_next_header = header_to_remove->next; new_next_header) {
                    new_next_header->prev = _h;
                }

                overhead_ -= management_block_size;
                remove_header_from_freelist(header_to_remove);
            }
        } // coalesce_with_next_unused_block

        auto remove_header_from_freelist(header* _h) -> void
        {
            const auto end = std::end(freelist_);

            if (const auto iter = std::find(std::begin(freelist_), end, _h); iter != end) {
                freelist_.erase(iter);
            }
        } // remove_header_from_freelist

        ByteRep* buffer_;
        std::size_t buffer_size_;
        std::size_t allocated_;
        std::size_t overhead_;
        header* headers_;
        std::vector<header*> freelist_;
    }; // fixed_buffer_resource
} // namespace irods::experimental::pmr

#endif // IRODS_FIXED_BUFFER_RESOURCE_HPP

