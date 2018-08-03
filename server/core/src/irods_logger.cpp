#include "irods_logger.hpp"

#include "irods_server_properties.hpp"

#include <unordered_map>

namespace irods::experimental {

void log::init() noexcept
{
    static const char* id = "";
    log_ = spdlog::syslog_logger("syslog", id, LOG_PID);
    log_->set_level(spdlog::level::trace); // Log everything!
}

auto log::to_level(const std::string& _level) -> log::level
{
    // clang-format off
    static const std::unordered_map<std::string, level> conv_table{
        {"trace",    level::trace},
        {"debug",    level::debug},
        {"info",     level::info},
        {"warn",     level::warn},
        {"error",    level::error},
        {"critical", level::critical}
    };
    // clang-format on

    if (auto iter = conv_table.find(_level); std::end(conv_table) != iter)
    {
        return iter->second;
    }

    return level::info;
}

auto log::get_level_from_config(const std::string& _category) -> log::level
{
    using map_t = std::unordered_map<std::string, boost::any>;
    const auto& log_level = irods::get_server_property<const map_t&>(irods::CFG_LOG_LEVEL_KW);
    return to_level(boost::any_cast<const std::string&>(log_level.at(_category)));
}

void log::set_error_object(rError_t* _error) noexcept
{
    error_ = _error;

    if (!_error)
    {
        write_to_error_object_ = false;
    }
}

void log::write_to_error_object(bool _value) noexcept
{
    write_to_error_object_ = _value;
}

} // namespace irods::experimental

