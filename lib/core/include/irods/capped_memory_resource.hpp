#ifndef IRODS_CAPPED_MEMORY_RESOURCE_HPP
#define IRODS_CAPPED_MEMORY_RESOURCE_HPP

/// \file

#include <boost/container/pmr/memory_resource.hpp>

#include "fmt/format.h"

#include <new>
#include <stdexcept>

/// A namespace containing components meant to be used with Boost.Container's PMR library.
namespace irods::experimental::pmr
{
    /// A \p capped_memory_resource is a special purpose memory resource class that allows
    /// applications to enforce a limit on the amount of memory that can be allocated via
    /// global operator new and delete.
    ///
    /// \since 4.2.11
    class capped_memory_resource
        : public boost::container::pmr::memory_resource
    {
    public:
        /// Constructs a \p capped_memory_resource with the given limit.
        ///
        /// \param[in] _max_size The maximum amount of bytes allowed to be allocated.
        ///
        /// \throws std::invalid_argument If any of the incoming constructor arguments do not
        ///                               satisfy construction requirements.
        ///
        /// \since 4.2.11
        explicit capped_memory_resource(std::int64_t _max_size)
            : boost::container::pmr::memory_resource{}
            , max_size_(_max_size)
            , allocated_{}
        {
            if (_max_size <= 0) {
                throw std::invalid_argument{
                    fmt::format("capped_memory_resource: max size must be greater than zero [max_size={}].", _max_size)};
            }
        } // capped_memory_resource

        capped_memory_resource(const capped_memory_resource&) = delete;
        auto operator=(const capped_memory_resource&) -> capped_memory_resource& = delete;

        ~capped_memory_resource() = default;

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

    protected:
        auto do_allocate(std::size_t _bytes, std::size_t _alignment) -> void* override
        {
            if (allocated_ + _bytes >= max_size_) {
                throw std::bad_alloc{};
            }

            auto* p = ::operator new(_bytes, std::align_val_t{_alignment});
            allocated_ += _bytes;

            return p;
        } // do_allocate

        auto do_deallocate(void* _p, std::size_t _bytes, std::size_t _alignment) -> void override
        {
            ::operator delete(_p, std::align_val_t{_alignment});
            allocated_ -= _bytes;
        } // do_deallocate

        auto do_is_equal(const boost::container::pmr::memory_resource&) const noexcept -> bool override
        {
            return true;
        } // do_is_equal

    private:
        std::size_t max_size_;
        std::size_t allocated_;
    }; // capped_memory_resource
} // namespace irods::experimental::pmr

#endif // IRODS_CAPPED_MEMORY_RESOURCE_HPP

