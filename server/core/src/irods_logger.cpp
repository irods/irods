#include "irods_logger.hpp"

#include "irods_server_properties.hpp"

#include <unordered_map>

namespace irods::experimental {

void log::init() noexcept
{
    static const char* id = "";
    log_ = spdlog::syslog_logger("syslog", id, LOG_PID, LOG_LOCAL0);
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

    if (auto iter = conv_table.find(_level); std::end(conv_table) != iter) {
        return iter->second;
    }

    return level::info;
}

auto log::get_level_from_config(const std::string& _category) -> log::level
{
    try {
        using map_type = std::unordered_map<std::string, boost::any>;
        const auto& log_level = irods::get_server_property<const map_type&>(irods::CFG_LOG_LEVEL_KW);
        return to_level(boost::any_cast<const std::string&>(log_level.at(_category)));
    }
    catch (const std::exception&) {
        log::server::warn({{"log_message", "Cannot get 'log_level' for log category. "
                                           "Defaulting to 'info'."},
                           {"requested_category",  _category}});
    }

    return log::level::info;
}

void log::set_error_object(rError_t* _error) noexcept
{
    error_ = _error;

    if (!_error) {
        write_to_error_object_ = false;
    }
}

void log::write_to_error_object(bool _value) noexcept
{
    write_to_error_object_ = _value;
}

void log::set_request_api_number(int _api_number) noexcept
{
    api_number_ = _api_number;
    log_api_number_ = true;
}

void log::clear_request_api_number() noexcept
{
    log_api_number_ = false;
}

void log::set_request_client_version(const version_t* _client_version) noexcept
{
    req_client_version_ = _client_version;
}

void log::set_request_client_host(std::string _host) noexcept
{
    req_client_host_ = std::move(_host);
}

void log::set_request_client_user(std::string _user) noexcept
{
    req_client_user_ = std::move(_user);
}

void log::set_request_proxy_user(std::string _user) noexcept
{
    req_proxy_user_ = std::move(_user);
}

void log::set_server_type(std::string _type) noexcept
{
    server_type_ = std::move(_type);
}

void log::set_server_host(std::string _host) noexcept
{
    server_host_ = std::move(_host);
}

void log::set_server_name(std::string _name) noexcept
{
    server_name_ = std::move(_name);
}

} // namespace irods::experimental

