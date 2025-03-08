#include "irods/access_time_manager.hpp"

#include <boost/interprocess/ipc/message_queue.hpp>
#include <unistd.h>

#include <memory>
#include <string>

namespace
{
    std::string g_mq_name; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    std::unique_ptr<boost::interprocess::message_queue> g_mq; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
} // anonymous namespace

namespace irods::access_time_manager
{
    auto init(const std::string_view _id, std::size_t _queue_size) -> void
    {
        g_mq_name = std::string{_id};

        // Allow an existing message queue to be opened. This allows access time
        // data to be processed acrossed server restarts.
        g_mq = std::make_unique<boost::interprocess::message_queue>(
            boost::interprocess::open_or_create, g_mq_name.data(), _queue_size, sizeof(access_time_data));
    }

    auto init_no_create(const std::string_view _id) -> void
    {
        g_mq_name = std::string{_id};
        g_mq = std::make_unique<boost::interprocess::message_queue>(boost::interprocess::open_only, g_mq_name.data());
    }

    auto try_enqueue(const access_time_data& _data) -> bool
    {
        return g_mq->try_send(&_data, sizeof(access_time_data), 0);
    }

    auto try_dequeue(access_time_data& _data) -> bool
    {
        [[maybe_unused]] boost::interprocess::message_queue::size_type recv_size; // NOLINT(cppcoreguidelines-init-variables)
        [[maybe_unused]] unsigned int priority; // NOLINT(cppcoreguidelines-init-variables)
        return g_mq->try_receive(&_data, sizeof(access_time_data), recv_size, priority);
    }
} // namespace irods::access_time_manager
