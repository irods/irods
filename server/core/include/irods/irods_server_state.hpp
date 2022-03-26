#ifndef IRODS_SERVER_STATE_HPP
#define IRODS_SERVER_STATE_HPP

#include "irods/irods_error.hpp"

#include <cstddef>
#include <string_view>

namespace irods::server_state
{
    using int_type = std::int8_t;

    enum class server_state
    {
        running = 0,
        paused,
        stopped,
        exited
    };

    auto init() -> void;

    auto deinit() -> void;

    auto get_state() -> server_state;

    auto set_state(server_state _new_state) -> irods::error;

    auto to_string(server_state _state) -> std::string_view;
} // namespace irods::server_state

#endif // IRODS_SERVER_STATE_HPP

