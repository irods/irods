#include "irods_logger.hpp"

#include "irods_server_properties.hpp"

#include <unordered_map>
#include <memory>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>

#include "boost/interprocess/sync/named_mutex.hpp"
#include "boost/interprocess/sync/scoped_lock.hpp"

#include <sys/types.h>
#include <unistd.h>

namespace ipc = boost::interprocess;

namespace irods::experimental {

class test_mode_ipc_sink
    : public spdlog::sinks::base_sink<spdlog::details::null_mutex>
{
public:
    test_mode_ipc_sink()
        : mutex_{ipc::open_or_create, shm_name}
        , file_{"/var/lib/irods/log/test_mode_output.log", std::ios_base::app}
        , owner_pid_{getpid()}
    {
    }

    test_mode_ipc_sink(const test_mode_ipc_sink&) = delete;
    test_mode_ipc_sink& operator=(const test_mode_ipc_sink&) = delete;

    ~test_mode_ipc_sink()
    {
        try {
            if (getpid() == owner_pid_) {
                ipc::named_mutex::remove(shm_name);
            }
        }
        catch (const ipc::interprocess_exception& e) {
            file_ << "ERROR: " << e.what() << std::endl;
        }
    }

protected:
    void _sink_it(const spdlog::details::log_msg& _msg) override
    {
        try {
            ipc::scoped_lock<ipc::named_mutex> lock{mutex_};
            file_ << _msg.raw.c_str() << std::endl;
        }
        catch (const ipc::interprocess_exception& e) {
            file_ << "ERROR: " << e.what() << std::endl;
        }
    }

    void _flush() override
    {
    }

private:
    inline static const char* const shm_name = "irods_test_mode_ipc_sink";

    ipc::named_mutex mutex_;
    std::ofstream file_;
    const pid_t owner_pid_;
};

class stdout_ipc_sink
    : public spdlog::sinks::base_sink<spdlog::details::null_mutex>
{
public:
    stdout_ipc_sink()
        : mutex_{ipc::open_or_create, shm_name}
        , owner_pid_{getpid()}
    {
    }

    stdout_ipc_sink(const stdout_ipc_sink&) = delete;
    stdout_ipc_sink& operator=(const stdout_ipc_sink&) = delete;

    ~stdout_ipc_sink()
    {
        try {
            if (getpid() == owner_pid_) {
                ipc::named_mutex::remove(shm_name);
            }
        }
        catch (const ipc::interprocess_exception& e) {
            std::cerr << "ERROR: " << e.what() << std::endl;
        }
    }

protected:
    void _sink_it(const spdlog::details::log_msg& _msg) override
    {
        try {
            ipc::scoped_lock<ipc::named_mutex> lock{mutex_};
            std::cout << _msg.raw.c_str() << std::endl;
        }
        catch (const ipc::interprocess_exception& e) {
            std::cerr << "ERROR: " << e.what() << std::endl;
        }
    }

    void _flush() override
    {
    }

private:
    inline static const char* const shm_name = "irods_stdout_ipc_sink";

    ipc::named_mutex mutex_;
    const pid_t owner_pid_;
};

void log::init(bool _write_to_stdout, bool _enable_test_mode) noexcept
{
    std::vector<spdlog::sink_ptr> sinks;

    if (_write_to_stdout) {
        sinks.push_back(std::make_shared<stdout_ipc_sink>());
    }
    else {
        static const char* id = "";
        sinks.push_back(std::make_shared<spdlog::sinks::syslog_sink>(id, LOG_PID, LOG_LOCAL0));
    }

    if (_enable_test_mode) {
        sinks.push_back(std::make_shared<test_mode_ipc_sink>());
    }

    log_ = std::make_shared<spdlog::logger>("composite_logger", std::begin(sinks), std::end(sinks));
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

