#include "irods/irods_logger.hpp"

#include "irods/irods_server_properties.hpp"
#include "irods/irods_default_paths.hpp"
#include "irods/rodsError.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#ifdef IRODS_ENABLE_SYSLOG
#  include <spdlog/sinks/base_sink.h>
#  include <spdlog/sinks/syslog_sink.h>
#endif // IRODS_ENABLE_SYSLOG

#include <sys/types.h>
#include <unistd.h>

namespace ipc = boost::interprocess;

namespace
{
    // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
#ifdef IRODS_ENABLE_SYSLOG
    std::shared_ptr<spdlog::logger> g_log;
#endif // IRODS_ENABLE_SYSLOG
    ErrorStack* g_error;
    bool g_write_to_error_object;
    int g_api_number;
    bool g_log_api_number;
    const Version* g_req_client_version;
    std::string g_req_client_host;
    std::string g_req_client_username;
    std::string g_req_proxy_username;
    std::string g_server_type;
    std::string g_server_host;
    std::string g_server_name;
    std::string g_server_zone;
    // NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
} // anonymous namespace

namespace irods::experimental::log
{
#ifdef IRODS_ENABLE_SYSLOG
    class test_mode_ipc_sink // NOLINT(cppcoreguidelines-special-member-functions)
        : public spdlog::sinks::base_sink<spdlog::details::null_mutex>
    {
      public:
        test_mode_ipc_sink()
            : mutex_{ipc::open_or_create, shm_name}
            , file_{(irods::get_irods_home_directory() / "log" / "test_mode_output.log").string(), std::ios_base::app}
            , owner_pid_{getpid()}
        {
        }

        test_mode_ipc_sink(const test_mode_ipc_sink&) = delete;
        auto operator=(const test_mode_ipc_sink&) -> test_mode_ipc_sink& = delete;

        ~test_mode_ipc_sink() // NOLINT(modernize-use-override)
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
        void sink_it_(const spdlog::details::log_msg& msg) override
        {
            try {
                ipc::scoped_lock<ipc::named_mutex> lock{mutex_};
                file_ << msg.payload.data() << std::endl;
            }
            catch (const ipc::interprocess_exception& e) {
                file_ << "ERROR: " << e.what() << std::endl;
            }
        }

        void flush_() override
        {
        }

      private:
        inline static const char* const shm_name = "irods_test_mode_ipc_sink";

        ipc::named_mutex mutex_;
        std::ofstream file_;
        const pid_t owner_pid_;
    }; // class test_mode_ipc_sink

    class stdout_ipc_sink // NOLINT(cppcoreguidelines-special-member-functions)
        : public spdlog::sinks::base_sink<spdlog::details::null_mutex>
    {
      public:
        stdout_ipc_sink()
            : mutex_{ipc::open_or_create, shm_name}
            , owner_pid_{getpid()}
        {
        }

        stdout_ipc_sink(const stdout_ipc_sink&) = delete;
        auto operator=(const stdout_ipc_sink&) -> stdout_ipc_sink& = delete;

        ~stdout_ipc_sink() // NOLINT(modernize-use-override)
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
        void sink_it_(const spdlog::details::log_msg& msg) override
        {
            try {
                ipc::scoped_lock<ipc::named_mutex> lock{mutex_};
                std::cout << msg.payload.data() << std::endl;
            }
            catch (const ipc::interprocess_exception& e) {
                std::cerr << "ERROR: " << e.what() << std::endl;
            }
        }

        void flush_() override
        {
        }

      private:
        inline static const char* const shm_name = "irods_stdout_ipc_sink";

        ipc::named_mutex mutex_;
        const pid_t owner_pid_;
    }; // class stdout_ipc_sink
#endif // IRODS_ENABLE_SYSLOG

    void init(bool _write_to_stdout, bool _enable_test_mode) noexcept
    {
#ifdef IRODS_ENABLE_SYSLOG
        std::vector<spdlog::sink_ptr> sinks;

        if (_write_to_stdout) {
            sinks.push_back(std::make_shared<stdout_ipc_sink>());
        }
        else {
            std::string id;
            const bool enable_formatting = false;
            // NOLINTNEXTLINE(hicpp-signed-bitwise)
            sinks.push_back(std::make_shared<spdlog::sinks::syslog_sink_mt>(id, LOG_PID, LOG_LOCAL0, enable_formatting));
        }

        if (_enable_test_mode) {
            sinks.push_back(std::make_shared<test_mode_ipc_sink>());
        }

        g_log = std::make_shared<spdlog::logger>("composite_logger", std::begin(sinks), std::end(sinks));
        g_log->set_level(spdlog::level::trace); // Log everything!
#endif // IRODS_ENABLE_SYSLOG
    }

    auto to_level(const std::string_view _level) noexcept -> level
    {
        // clang-format off
        static const std::unordered_map<std::string_view, level> conv_table{
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

    auto get_level_from_config(const std::string_view _category) noexcept -> level
    {
        try {
            const auto& log_level = irods::get_server_property<const nlohmann::json&>(irods::KW_CFG_LOG_LEVEL);
            return to_level(log_level.at(_category.data()).get_ref<const std::string&>());
        }
        catch (...) {
            try {
                server::trace("Cannot get log level for log category [{}]. Defaulting to [info].", _category);
            }
            catch (...) {
            }
        }

        return level::info;
    }

    auto set_error_object(ErrorStack* _error) noexcept -> void
    {
        g_error = _error;

        // NOLINTNEXTLINE(readability-implicit-bool-conversion)
        if (!_error) {
            g_write_to_error_object = false;
        }
    }

    auto get_error_object() noexcept -> ErrorStack*
    {
        return g_error;
    }

    auto write_to_error_object(bool _value) noexcept -> void
    {
        g_write_to_error_object = _value;
    }

    auto should_write_to_error_object() noexcept -> bool
    {
        return g_write_to_error_object;
    }

    auto set_request_api_number(int _api_number) noexcept -> void
    {
        g_api_number = _api_number;
        g_log_api_number = true;
    }

    auto clear_request_api_number() noexcept -> void
    {
        g_log_api_number = false;
    }

    auto get_request_api_number() noexcept -> std::optional<int>
    {
        if (g_log_api_number) {
            return g_api_number;
        }

        return std::nullopt;
    }

    auto set_request_client_version(const Version* _client_version) noexcept -> void
    {
        g_req_client_version = _client_version;
    }

    auto get_request_client_version() noexcept -> const Version*
    {
        return g_req_client_version;
    }

    auto set_request_client_hostname(std::string _hostname) noexcept -> void
    {
        g_req_client_host = std::move(_hostname);
    }

    auto get_request_client_hostname() noexcept -> std::string_view
    {
        return g_req_client_host;
    }

    auto set_request_client_username(std::string _username) noexcept -> void
    {
        g_req_client_username = std::move(_username);
    }

    auto get_request_client_username() noexcept -> std::string_view
    {
        return g_req_client_username;
    }

    auto set_request_proxy_username(std::string _username) noexcept -> void
    {
        g_req_proxy_username = std::move(_username);
    }

    auto get_request_proxy_username() noexcept -> std::string_view
    {
        return g_req_proxy_username;
    }

    auto set_server_type(std::string _type) noexcept -> void
    {
        g_server_type = std::move(_type);
    }

    auto get_server_type() noexcept -> std::string_view
    {
        return g_server_type;
    }

    auto set_server_hostname(std::string _hostname) noexcept -> void
    {
        g_server_host = std::move(_hostname);
    }

    auto get_server_hostname() noexcept -> std::string_view
    {
        return g_server_host;
    }

    auto set_server_name(std::string _name) noexcept -> void
    {
        g_server_name = std::move(_name);
    }

    auto get_server_name() noexcept -> std::string_view
    {
        return g_server_name;
    }

    auto set_server_zone(std::string _name) noexcept -> void
    {
        g_server_zone = std::move(_name);
    }

    auto get_server_zone() noexcept -> std::string_view
    {
        return g_server_zone;
    }

    namespace detail
    {
#ifdef IRODS_ENABLE_SYSLOG
        auto get_logger() noexcept -> std::shared_ptr<spdlog::logger>
        {
            return g_log;
        }
#endif // IRODS_ENABLE_SYSLOG
    } // namespace detail
} // namespace irods::experimental::log
