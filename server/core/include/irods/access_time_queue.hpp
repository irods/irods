#ifndef IRODS_ACCESS_TIME_QUEUE_HPP
#define IRODS_ACCESS_TIME_QUEUE_HPP

/// \file

#include <cstddef>
#include <ctime>
#include <string_view>

/// A namespace containing facilities for managing access time updates for replicas.
///
/// \since 5.0.0
namespace irods::access_time_queue
{
    /// A data type which holds information for updating the access time of a replica.
    ///
    /// \since 5.0.0
    struct access_time_data
    {
        std::size_t data_id;
        std::size_t replica_number;
        char last_accessed[12]; // 11 digits + null terminating byte
    }; // struct access_time_data

    /// Initializes the access time queue.
    ///
    /// This function should only be called on startup of the server.
    ///
    /// \param[in] _queue_name_prefix The prefix of the queue name.
    /// \param[in] _queue_size        The max number of entries the queue can hold.
    ///
    /// \throws irods::exception If the queue name is empty or the queue size violates the range [0, 500000).
    ///
    /// \since 5.0.0
    auto init(const std::string_view _queue_name_prefix, std::int32_t _queue_size) -> void;

    /// Initializes an existing access time queue.
    ///
    /// This function should only be called on startup of the server.
    ///
    /// \param[in] _queue_name The name of an existing queue.
    ///
    /// \since 5.0.0
    auto init_no_create(const std::string_view _queue_name) -> void;

    /// Cleans up any resources created via init().
    ///
    /// This function must be called from the same process that called init().
    ///
    /// \since 5.0.0
    auto deinit() noexcept -> void;

    /// Returns the name of the shared memory segment used by the access time queue.
    ///
    /// This function allows other components and processes to use an existing access time queue.
    ///
    /// \since 5.0.0
    auto shared_memory_name() -> std::string_view;

    /// Returns true if new access time data was added to the queue, otherwise false.
    ///
    /// \p false is always returned when the queue is full. This function call returns
    /// immediately and does not block.
    ///
    /// \since 5.0.0
    auto try_enqueue(const access_time_data& _data) -> bool;

    /// Returns true if access time data was removed from the queue, otherwise false.
    ///
    /// \p false is always returned when the queue is empty. This function call returns
    /// immediately and does not block.
    ///
    /// \since 5.0.0
    auto try_dequeue(access_time_data& _data) -> bool;

    /// Returns the number of elements in the queue.
    ///
    /// \since 5.0.0
    auto number_of_queued_updates() -> std::size_t;
} // namespace irods::access_time_queue

#endif // IRODS_ACCESS_TIME_QUEUE_HPP
