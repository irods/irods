#ifndef IRODS_ACCESS_TIME_MANAGER_HPP
#define IRODS_ACCESS_TIME_MANAGER_HPP

/// \file

#include <cstddef>
#include <ctime>
#include <string_view>

/// TODO
///
/// \since 5.0.0
namespace irods::access_time_manager
{
    /// TODO
    ///
    /// \since 5.0.0
    struct access_time_data
    {
        std::size_t data_id;
        std::size_t replica_number;
        std::time_t last_accessed;
    }; // struct access_time_update

    /// Initializes the access time manager.
    ///
    /// This function should only be called on startup of the server.
    ///
    /// \param[in] _id         The name of the manager.
    /// \param[in] _queue_size The max number of entries the queue can hold.
    ///
    /// \since 5.0.0
    auto init(const std::string_view _id, std::size_t _queue_size) -> void;

    /// Initializes an existing access time manager.
    ///
    /// This function should only be called on startup of the server.
    ///
    /// \since 5.0.0
    auto init_no_create(const std::string_view _id) -> void;

    /// TODO
    ///
    /// \since 5.0.0
    auto try_enqueue(const access_time_data& _data) -> bool;

    /// TODO
    ///
    /// \since 5.0.0
    auto try_dequeue(access_time_data& _data) -> bool;
} // namespace irods::access_time_manager

#endif // IRODS_ACCESS_TIME_MANAGER_HPP
