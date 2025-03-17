#include "irods/access_time_queue.hpp"

#include "irods/irods_exception.hpp"
#include "irods/rodsErrorTable.h"

#include <boost/interprocess/ipc/message_queue.hpp>
#include <fmt/format.h>
#include <unistd.h>

#include <cctype>
#include <chrono>
#include <memory>
#include <string>

namespace
{
    //
    // Global Variables
    //

    // On initialization, holds the PID of the process that initialized the access time queue.
    // This ensures that only the process that initialized the system can deinitialize it.
    pid_t g_owner_pid; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    // Holds the name of the message queue which will exist in shared memory.
    std::string g_mq_name; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    // The pointer to the message queue which will exist in shared memory.
    // Allocating on the heap allows us to know when the message queue is constructed/destructed.
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    std::unique_ptr<boost::interprocess::message_queue> g_mq;

    // Returns true if the string starts with an alphabetic character or underscore, optionally followed
    // by one or more alphanumeric characters and/or underscore. The string is expected to contain ASCII
    // characters only. Anything else is considered undefined behavior.
    auto is_queue_name_prefix_valid(const std::string_view _s) -> bool
    {
        if (const auto first_ch = _s[0]; std::isalpha(static_cast<unsigned char>(first_ch)) == 0 && first_ch != '_') {
            return false;
        }

        return std::all_of(std::next(std::begin(_s)), std::end(_s), [](const unsigned char _ch) {
            return _ch == '_' || std::isalnum(_ch);
        });
    } // is_queue_name_prefix_valid
} // anonymous namespace

namespace irods::access_time_queue
{
    auto init(const std::string_view _queue_name_prefix, std::int32_t _queue_size) -> void
    {
        if (getpid() == g_owner_pid) {
            return;
        }

        if (_queue_name_prefix.empty()) {
            THROW(CONFIGURATION_ERROR, fmt::format("{}: Access time queue name prefix is empty.", __func__));
        }

        if (!is_queue_name_prefix_valid(_queue_name_prefix)) {
            THROW(CONFIGURATION_ERROR,
                  fmt::format(
                      "{}: Access time queue name prefix violates name requirement: [_a-zA-Z][_a-zA-Z0-9]*", __func__));
        }

        if (_queue_size < 0) {
            THROW(CONFIGURATION_ERROR,
                  fmt::format("{}: Access time queue size [{}] is less than 0.", __func__, _queue_size));
        }

        if (constexpr std::int32_t max = 500'000; _queue_size > max) {
            THROW(CONFIGURATION_ERROR,
                  fmt::format("{}: Access time queue size [{}] is greater than {}.", __func__, _queue_size, max));
        }

        using clock_type = std::chrono::system_clock;
        using seconds = std::chrono::seconds;

        const auto epoch = duration_cast<seconds>(clock_type::now().time_since_epoch()).count();
        g_mq_name = fmt::format("{}{}_{}", _queue_name_prefix, getpid(), epoch);

        boost::interprocess::message_queue::remove(g_mq_name.c_str());

        g_owner_pid = getpid();
        g_mq = std::make_unique<boost::interprocess::message_queue>(
            boost::interprocess::create_only, g_mq_name.data(), _queue_size, sizeof(access_time_data));
    } // init

    auto init_no_create(const std::string_view _queue_name) -> void
    {
        if (_queue_name.empty()) {
            THROW(CONFIGURATION_ERROR, fmt::format("{}: Access time queue name is empty.", __func__));
        }

        g_owner_pid = 0;
        g_mq_name = std::string{_queue_name};
        g_mq = std::make_unique<boost::interprocess::message_queue>(boost::interprocess::open_only, g_mq_name.data());
    } // init_no_create

    auto deinit() noexcept -> void
    {
        if (getpid() != g_owner_pid) {
            return;
        }

        try {
            g_owner_pid = 0;

            if (g_mq) {
                g_mq.reset();
            }

            boost::interprocess::message_queue::remove(g_mq_name.c_str());
        }
        catch (...) {
        }
    } // deinit

    auto shared_memory_name() -> std::string_view
    {
        return g_mq_name;
    } // shared_memory_name

    auto try_enqueue(const access_time_data& _data) -> bool
    {
        return g_mq->try_send(&_data, sizeof(access_time_data), 0);
    } // try_enqueue

    auto try_dequeue(access_time_data& _data) -> bool
    {
        // NOLINTNEXTLINE(cppcoreguidelines-init-variables)
        [[maybe_unused]] boost::interprocess::message_queue::size_type recv_size;
        [[maybe_unused]] unsigned int priority; // NOLINT(cppcoreguidelines-init-variables)
        return g_mq->try_receive(&_data, sizeof(access_time_data), recv_size, priority);
    } // try_dequeue

    auto number_of_queued_updates() -> std::size_t
    {
        return g_mq->get_num_msg();
    } // number_of_queued_updates
} // namespace irods::access_time_queue
