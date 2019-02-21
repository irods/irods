#ifndef IRODS_EXPERIMENTAL_LOGGER_HPP
#define IRODS_EXPERIMENTAL_LOGGER_HPP

#ifndef SPDLOG_ENABLE_SYSLOG
#define SPDLOG_ENABLE_SYSLOG
#endif

#include <unistd.h>
#include <sys/time.h>

#include <memory>
#include <string>
#include <utility>
#include <initializer_list>
#include <type_traits>
#include <iterator>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <chrono>

#include <spdlog.h>
#include <json.hpp>

#include <boost/range/iterator_range_core.hpp>

#include "apiNumberMap.h"
#include "rodsError.h"
#include "rcMisc.h"
#include "rcConnect.h"

namespace irods::experimental {

class log
{
public:
    using key_value = std::pair<const std::string, std::string>;

    enum class level : std::uint8_t
    {
        trace,
        debug,
        info,
        warn,
        error,
        critical
    };

    struct category
    {
        struct legacy;
        struct server;
        struct agent_factory;
        struct agent;
        struct delay_server;
        struct resource;
        struct database;
        struct authentication;
        struct api;
        struct microservice;
        struct network;
        struct rule_engine;
    };

    template <typename Category> class logger_config;
    template <typename Category> class logger;

    // clang-format off
    using legacy         = logger<category::legacy>;
    using server         = logger<category::server>;
    using agent_factory  = logger<category::agent_factory>;
    using agent          = logger<category::agent>;
    using delay_server   = logger<category::delay_server>;
    using resource       = logger<category::resource>;
    using database       = logger<category::database>;
    using authentication = logger<category::authentication>;
    using api            = logger<category::api>;
    using microservice   = logger<category::microservice>;
    using network        = logger<category::network>;
    using rule_engine    = logger<category::rule_engine>;
    // clang-format on

    log() = delete;

    log(const log&) = delete;
    log& operator=(const log&) = delete;

    static void init() noexcept;
    static auto to_level(const std::string& _level) -> level;
    static auto get_level_from_config(const std::string& _category) -> level;
    static void set_error_object(rError_t* _error) noexcept;
    static void write_to_error_object(bool _value) noexcept;
    static void set_request_api_number(int _api_number) noexcept;
    static void clear_request_api_number() noexcept;
    static void set_request_client_version(const version_t* _client_version) noexcept;
    static void set_request_client_host(std::string _host) noexcept;
    static void set_request_client_user(std::string _user) noexcept;
    static void set_request_proxy_user(std::string _user) noexcept;
    static void set_server_type(std::string _type) noexcept;
    static void set_server_host(std::string _host) noexcept;
    static void set_server_pid(int _pid) noexcept;
    static void set_server_name(std::string _name) noexcept;

    template <typename Category>
    class logger
    {
    public:
        template <level> class impl;

        logger() = delete;

        logger(const logger&) = delete;
        logger& operator=(const logger&) = delete;

        static void set_level(level _level) noexcept;

        // clang-format off
        inline static const auto trace    = impl<level::trace>{};
        inline static const auto debug    = impl<level::debug>{};
        inline static const auto info     = impl<level::info>{};
        inline static const auto warn     = impl<level::warn>{};
        inline static const auto error    = impl<level::error>{};
        inline static const auto critical = impl<level::critical>{};
        // clang-format on
    };

private:
    inline static std::shared_ptr<spdlog::logger> log_{};
    inline static rError_t* error_{};
    inline static bool write_to_error_object_{};
    inline static int api_number_{};
    inline static bool log_api_number_{};
    inline static const version_t* req_client_version_{};
    inline static std::string req_client_host_{};
    inline static std::string req_client_user_{};
    inline static std::string req_proxy_user_{};
    inline static std::string server_type_{};
    inline static std::string server_host_{};
    inline static int server_pid_{};
    inline static std::string server_name_{};
};

#include "irods_logger.tpp"

} // namespace irods::experimental

#endif // IRODS_EXPERIMENTAL_LOGGER_HPP

