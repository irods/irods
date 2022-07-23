#ifndef IRODS_LOGGER_HPP
#define IRODS_LOGGER_HPP

/// \file
/// \brief The iRODS Logging library
///
/// This library is designed for server-side use only. It is tightly coupled to the server and
/// makes use of features such as syslog and shared memory. Attempting to use this library in client-side
/// code is highly discouraged.
///
/// To use it, the following requirements must be satisfied.
/// - Define the macro, \p IRODS_ENABLE_SYSLOG, before including the header file
/// - Link against \p libirods_common.so
/// - Link against \p libfmt.so (must be from the same externals package used to compile \p libirods_common.so)
///
/// If you're implementing plugins for the iRODS server, then the requirements just mentioned can be skipped.

#ifdef IRODS_ENABLE_SYSLOG
#  define SPDLOG_ENABLE_SYSLOG
#  include <spdlog/spdlog.h>
#  undef SPDLOG_ENABLE_SYSLOG
#endif // IRODS_ENABLE_SYSLOG

#include "irods/apiNumberMap.h"
#include "irods/rodsError.h"
#include "irods/rcMisc.h"
#include "irods/rcConnect.h"

#include <boost/range/iterator_range_core.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <unistd.h>
#include <sys/time.h>

#include <ctime>
#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <initializer_list>
#include <type_traits>
#include <iterator>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <concepts>

/// Defines all things related to the logging API.
///
/// \since 4.3.0
namespace irods::experimental::log
{
    /// A type alias used to represent a single JSON property in a log message.
    ///
    /// \since 4.3.0
    using key_value = std::pair<const std::string, std::string>;

    /// An enumeration representing the list of supported log levels.
    ///
    /// The levels are ordered from most noisy to least noisy.
    ///
    /// \since 4.3.0
    enum class level
    {
        trace,
        debug,
        info,
        warn,
        error,
        critical
    }; // enum class level

    /// Pre-defined log categories.
    ///
    /// The types defined within this namespace serve as tags. They are used to tie various logging
    /// components together.
    ///
    /// \since 4.3.0
    namespace category
    {
        // clang-format off
        struct legacy {};
        struct server {};
        struct agent_factory {};
        struct agent {};
        struct delay_server {};
        struct resource {};
        struct database {};
        struct authentication {};
        struct api {};
        struct microservice {};
        struct network {};
        struct rule_engine {};
        struct sql {};
        // clang-format on
    } // namespace category

    // clang-format off
    /// A class template forward declaration that allows users to introduce new log categories.
    ///
    /// This class template is meant to be specialized based on a log category. Pre-defined
    /// specializations have been provided for the log categories defined in the category namespace.
    ///
    /// \tparam Category A tag type used to associate state to a particular log category.
    ///
    /// Below is an example that demonstrates how to specialize this class template.
    /// \code{.cpp}
    /// struct my_log_category; // My fancy new log category.
    ///
    /// template <>
    /// class logger_config<my_log_category>
    /// {
    ///     static constexpr const char* name = "my_log_category"; // The name you want to display in the log message.
    ///     inline static level level = level::info;               // The default log level for the log category.
    ///
    ///     friend class logger<my_log_category>;
    /// }; // class logger_config<category::legacy>
    /// \endcode
    ///
    /// \since 4.3.0
    template <typename Category> class logger_config;

    // A forward declaration used to define type aliases for various loggers.
    // See implementation of class template for more information.
    template <typename Category> class logger;

    /// \name Log Categories
    ///
    /// Pre-defined type alias representing loggers that are meant to be used in various components
    /// throughout the iRODS codebase.
    ///
    /// The names of each type alias act as a hint for where they should be used. For example, the
    /// \p microservice logger should be used in microservice specific code. Choosing not to follow
    /// this recommendation will result in confusing log messages.
    ///@{
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
    using sql            = logger<category::sql>;
    ///@}
    // clang-format on

    /// Initializes logging facilities.
    ///
    /// This function must be called before invoking other logging API operations.
    /// This function is not thread-safe.
    ///
    /// \param[in] _write_to_stdout  Configures the logging API to write all messages to stdout.
    /// \param[in] _enable_test_mode Configures the logging API to also write messages to a special
    ///                              file that flushes its output on every write. This is good for
    ///                              tests that need to search the log file for specific messages.
    ///
    /// \since 4.3.0
    auto init(bool _write_to_stdout = false, bool _enable_test_mode = false) noexcept -> void;

    /// Converts a string to a specific log level.
    ///
    /// \param[in] _level The string representation of a supported log level.
    ///
    /// \return A \p level enumeration if the string is valid.
    /// \retval level::info If \p _level could not be converted to a supported log level.
    ///
    /// \since 4.3.0
    auto to_level(const std::string_view _level) noexcept -> level;

    /// Loads the log level for a specific log category from server_config.json.
    ///
    /// \param[in] _category The log category to fetch the log level for.
    ///
    /// \return A \p level enumeration if the log category is defined.
    /// \retval level::info If the log level cannot be retrieved for \p _category.
    ///
    /// \since 4.3.0
    auto get_level_from_config(const std::string_view _category) noexcept -> level;

    /// Associates or disassociates an ErrorStack object with all loggers.
    ///
    /// This function is not thread-safe.
    ///
    /// \param[in] _error A pointer to an ErrorStack object. Passing \p nullptr disassociates any
    ///                   error objects from the loggers.
    ///
    /// \since 4.3.0
    auto set_error_object(ErrorStack* _error) noexcept -> void;

    /// Returns the error object currently used by all loggers, if available.
    ///
    /// This function is not thread-safe.
    ///
    /// \since 4.3.1
    auto get_error_object() noexcept -> ErrorStack*;

    /// Instructs the logging library to write messages to an ErrorStack object if available.
    ///
    /// This function is not thread-safe.
    ///
    /// \param[in] _value A boolean which enables or disables writing messages to the attached
    ///                   ErrorStack object.
    ///
    /// \see set_error_object()
    ///
    /// \since 4.3.0
    auto write_to_error_object(bool _value) noexcept -> void;

    /// Returns whether all loggers should write error messages to the error object.
    ///
    /// This function is not thread-safe.
    ///
    /// \since 4.3.1
    auto should_write_to_error_object() noexcept -> bool;

    /// Sets the API number for subsequent log messages.
    ///
    /// The value for this should be updated every time the connected client invokes an API endpoint.
    /// This function is not thread-safe.
    ///
    /// \param[in] _api_number The integer value mapped to a specific API endpoint.
    ///
    /// \since 4.3.0
    auto set_request_api_number(int _api_number) noexcept -> void;

    /// Instructs the logging library to not include the API number in subsequent log messages.
    ///
    /// This function is not thread-safe.
    ///
    /// \since 4.3.0
    auto clear_request_api_number() noexcept -> void;

    /// Returns the API number currently used by all loggers, if available.
    ///
    /// This function is not thread-safe.
    ///
    /// \since 4.3.1
    auto get_request_api_number() noexcept -> std::optional<int>;

    /// Sets the version information of the connected client for subsequent log messages.
    ///
    /// This function is not thread-safe.
    ///
    /// \param[in] _client_version A pointer to the version information of the connected client.
    ///                            Passing \p nullptr will cause all loggers to not include version
    ///                            information in subsequent log messages.
    ///
    /// \since 4.3.0
    auto set_request_client_version(const Version* _client_version) noexcept -> void;

    /// Returns the version information of the connected client currently used by all loggers.
    ///
    /// This function is not thread-safe.
    ///
    /// \since 4.3.1
    auto get_request_client_version() noexcept -> const Version*;

    /// Sets the hostname/IP of the connected client for subsequent log messages.
    ///
    /// This function is not thread-safe.
    ///
    /// \param[in] _host A string representing the hostname or IP of the connected client.
    ///
    /// \since 4.3.0
    auto set_request_client_hostname(std::string _hostname) noexcept -> void;

    /// Returns the client hostname of the connected client currently used by all loggers.
    ///
    /// This function is not thread-safe.
    ///
    /// \since 4.3.1
    auto get_request_client_hostname() noexcept -> std::string_view;

    /// Sets the username of the connected client for subsequent log messages.
    ///
    /// This function is not thread-safe.
    ///
    /// \param[in] _username A string containing the iRODS username of the connected client.
    ///
    /// \since 4.3.0
    auto set_request_client_username(std::string _username) noexcept -> void;

    /// Returns the client username of the connected client currently used by all loggers.
    ///
    /// This function is not thread-safe.
    ///
    /// \since 4.3.1
    auto get_request_client_username() noexcept -> std::string_view;

    /// Sets the proxy username of the connected client for subsequent log messages.
    ///
    /// This function is not thread-safe.
    ///
    /// \param[in] _username A string containing the iRODS proxy username of the connected client.
    ///
    /// \since 4.3.0
    auto set_request_proxy_username(std::string _username) noexcept -> void;

    /// Returns the proxy username of the connected client currently used by all loggers.
    ///
    /// This function is not thread-safe.
    ///
    /// \since 4.3.1
    auto get_request_proxy_username() noexcept -> std::string_view;

    /// Sets the server type for subsequent log messages.
    ///
    /// This function is not thread-safe.
    ///
    /// \param[in] _type A string representing the type of the server.
    ///
    /// \since 4.3.0
    auto set_server_type(std::string _type) noexcept -> void;

    /// Returns the server type currently used by all loggers.
    ///
    /// This function is not thread-safe.
    ///
    /// \since 4.3.1
    auto get_server_type() noexcept -> std::string_view;

    /// Sets the hostname of the iRODS server servicing the API request for subsequent log messages.
    ///
    /// This function is not thread-safe.
    ///
    /// \param[in] _host A string containing the hostname of the iRODS server.
    ///
    /// \since 4.3.0
    auto set_server_hostname(std::string _hostname) noexcept -> void;

    /// Returns the hostname of the iRODS server currently used by all loggers.
    ///
    /// This function is not thread-safe.
    ///
    /// \since 4.3.1
    auto get_server_hostname() noexcept -> std::string_view;

    /// Sets the server name for subsequent log messages.
    ///
    /// This function is not thread-safe.
    ///
    /// \param[in] _name A string representing the server name.
    ///
    /// \since 4.3.0
    auto set_server_name(std::string _name) noexcept -> void;

    /// Returns the server name currently used by all loggers.
    ///
    /// This function is not thread-safe.
    ///
    /// \since 4.3.1
    auto get_server_name() noexcept -> std::string_view;

    /// Defines types and functions that are private to the implementation of the library.
    ///
    /// Users of this library MUST NOT use anything defined in this namespace.
    ///
    /// \since 4.3.1
    namespace detail
    {
#ifdef IRODS_ENABLE_SYSLOG
        auto get_logger() noexcept -> std::shared_ptr<spdlog::logger>;
#endif // IRODS_ENABLE_SYSLOG
    } // namespace detail

    /// Defines various property names used to construct a log message.
    ///
    /// \since 4.3.0
    namespace tag
    {
        /// Defines various property names used to construct a log message.
        ///
        /// \since 4.3.0
        namespace log
        {
            // clang-format off
            inline constexpr const char* const category = "log_category";
            inline constexpr const char* const facility = "log_facility";
            inline constexpr const char* const message  = "log_message";
            inline constexpr const char* const level    = "log_level";
            // clang-format on
        } // namespace log

        /// Defines various property names used to construct a log message.
        ///
        /// \since 4.3.0
        namespace request
        {
            // clang-format off
            inline constexpr const char* const release_version = "request_release_version";
            inline constexpr const char* const api_version     = "request_api_version";
            inline constexpr const char* const host            = "request_host";
            inline constexpr const char* const client_user     = "request_client_user";
            inline constexpr const char* const proxy_user      = "request_proxy_user";
            inline constexpr const char* const api_number      = "request_api_number";
            inline constexpr const char* const api_name        = "request_api_name";
            // clang-forexprmat on
        } // namespace request

        /// Defines various property names used to construct a log message.
        ///
        /// \since 4.3.0
        namespace server
        {
            // clang-format off
            inline constexpr const char* const type      = "server_type";
            inline constexpr const char* const host      = "server_host";
            inline constexpr const char* const pid       = "server_pid";
            inline constexpr const char* const name      = "server_name";
            inline constexpr const char* const timestamp = "server_timestamp";
            // clang-format on
        } // namespace server
    } // namespace tag

    //
    // Logger Interface Definition
    //

    /// A class template which defines the primary functionality of all loggers.
    ///
    /// Instantiations of this class template are not copyable or moveable.
    ///
    /// \tparam Category A tag type used to associate a logger to a particular log category.
    ///
    /// \since 4.3.0
    template <typename Category>
    class logger // NOLINT(cppcoreguidelines-special-member-functions)
    {
      public:
        /// A class template that provides the implementation for logging messages in various ways.
        ///
        /// Instantiations of this class template are not copyable or moveable. Users of the logging
        /// library MUST NOT instantiate this template directly. Future versions of this library are
        /// free to change the implementation of this class template.
        ///
        /// \tparam Level The level to specialize the implementation on.
        ///
        /// \since 4.3.0
        template <level Level>
        class impl // NOLINT(cppcoreguidelines-special-member-functions)
        {
          public:
            friend class logger<Category>;

            impl(const impl&) = delete;
            auto operator=(const impl&) -> impl& = delete;

#ifdef IRODS_ENABLE_SYSLOG
            template <typename T>
            using is_iterable = decltype(std::begin(std::declval<std::decay_t<T>>()));

            /// Writes a formatted string to the log file.
            ///
            /// \tparam Args The list of types matching the arguments passed.
            ///
            /// \param[in] _format The format string to write. It can include any number of placeholders.
            /// \param[in] _args   The format arguments to inject into the format string.
            ///
            /// \since 4.3.0
            template <typename... Args>
            auto operator()(fmt::format_string<Args...> _format, Args&&... _args) const -> void
            {
                if (should_log()) {
                    const auto msg = {key_value{tag::log::message, fmt::format(_format, std::forward<Args>(_args)...)}};
                    log_message(std::begin(msg), std::end(msg));
                }
            } // operator()

            /// Writes a string to the log file.
            ///
            /// \param[in] _msg The string to write to the log file.
            ///
            /// \since 4.3.0
            auto operator()(const std::string& _msg) const -> void
            {
                (*this)({{tag::log::message, _msg}});
            } // operator()

            /// Writes a list of ::key_value objects to the log file.
            ///
            /// This member function allows developers to inject new JSON properties into the log
            /// message. Keys become JSON property names that sit at the same level as ones automatically
            /// included by the logging API (e.g. "log_message" and "server_timestamp").
            ///
            /// This member function does not format any arguments passed to it.
            ///
            /// \param[in] _list The list of key-value pairs to write.
            ///
            /// \since 4.3.0
            auto operator()(std::initializer_list<key_value> _list) const -> void
            {
                (*this)(std::begin(_list), std::end(_list));
            } // operator()

            /// Writes the contents of a container to the log file.
            ///
            /// The container is required to hold a list of ::key_value objects.
            ///
            /// \tparam Container A container type that supports iteration of its elements.
            /// \tparam ValueType Must match ::key_value.
            ///
            /// \param[in] _container The container holding the list of ::key_value objects to write.
            ///
            /// \since 4.3.0
            template <
                typename Container,
                typename ValueType = typename Container::value_type,
                typename = is_iterable<Container>,
                typename = std::enable_if_t<std::is_same_v<ValueType, key_value>>>
            auto operator()(const Container& _container) const -> void
            {
                (*this)(std::begin(_container), std::end(_container));
            } // operator()

            /// Writes the values denoted by the range [_first, _last) to the log file.
            ///
            /// \tparam ForwardIt An iterator type that satifies the forward iterator requirements.
            /// \tparam ValueType Must match ::key_value.
            ///
            /// \param[in] _first The iterator representing the start of the range.
            /// \param[in] _last  The iterator representing the end of the range.
            ///
            /// \since 4.3.0
            template <
                typename ForwardIt,
                typename ValueType = typename std::iterator_traits<ForwardIt>::value_type,
                typename = std::enable_if_t<std::is_same_v<ValueType, key_value>>>
            auto operator()(ForwardIt _first, ForwardIt _last) const -> void
            {
                if (should_log()) {
                    log_message(_first, _last);
                }
            } // operator()

            /// Writes a formatted string to the log file.
            ///
            /// This member function gives developers a way to defer evaluation of format arguments. For
            /// those situations where it is expensive to construct objects and the logger decides to not
            /// write the results to the log file, this overload should be used.
            ///
            /// Below is an example demonstrating usage.
            /// \code{.cpp}
            /// using log_db = irods::experimental::log::database;
            ///
            /// log_db::info("This value, {}, is too expensive to construct for the logger to ignore.", [] {
            ///     return std::make_tuple(very_expensive_result_to_compute_and_ignore());
            /// });
            /// \endcode
            ///
            /// \param[in] _format    The format string to write.
            /// \param[in] _invocable An invocable object that returns a tuple-like object containing the
            ///                       arguments that will be injected into the format string.
            ///
            /// \since 4.3.1
            auto operator()(const std::string_view _format, std::invocable auto&& _invocable) const -> void
            {
                if (should_log()) {
                    std::apply(
                        [this, _format]<typename... Ts>(Ts&&... _args) {
                            const auto msg = {key_value{
                                tag::log::message, fmt::format(fmt::runtime(_format), std::forward<Ts>(_args)...)}};
                            log_message(std::begin(msg), std::end(msg));
                        },
                        _invocable());
                }
            } // operator()
#else // IRODS_ENABLE_SYSLOG
            // clang-format off

            // Clients should not have access to the logger, therefore we provide a
            // different implementation to allow inclusion of the logger without
            // breaking any existing implementation.

            // clang-format on

            constexpr auto operator()(std::initializer_list<key_value> _list) const noexcept -> void
            {
            } // operator()

            template <typename... Args>
            constexpr auto operator()(Args&&...) const noexcept -> void
            {
            } // operator()
#endif // IRODS_ENABLE_SYSLOG

          private:
            impl() = default;

#ifdef IRODS_ENABLE_SYSLOG
            [[nodiscard]] auto should_log() const noexcept -> bool
            {
                return Level >= logger_config<Category>::level;
            } // should_log

            [[nodiscard]] auto utc_timestamp() const -> std::string
            {
                // clang-format off
                using clock_type      = std::chrono::system_clock;
                using time_point_type = std::chrono::time_point<clock_type>;
                // clang-format on

                timespec ts; // NOLINT(cppcoreguidelines-pro-type-member-init)

                if (const auto ec = clock_gettime(CLOCK_REALTIME, &ts); ec != 0) {
                    const auto now = clock_type::to_time_t(clock_type::now());
                    std::stringstream utc_ss;
                    utc_ss << std::put_time(std::gmtime(&now), "%FT%T.000Z"); // NOLINT(concurrency-mt-unsafe)
                    return utc_ss.str();
                }

                const auto now = clock_type::to_time_t(time_point_type{std::chrono::seconds{ts.tv_sec}});

                std::stringstream utc_ss;
                utc_ss << std::put_time(std::gmtime(&now), "%FT%T"); // NOLINT(concurrency-mt-unsafe)

                const auto nsecs = std::chrono::nanoseconds{ts.tv_nsec};
                const auto msecs = std::chrono::duration_cast<std::chrono::milliseconds>(nsecs);

                return fmt::format("{}.{:0>3}Z", utc_ss.str(), msecs.count());
            } // utc_timestamp

            static constexpr auto log_level_as_string() noexcept -> const char*
            {
                // clang-format off
                if      constexpr (Level == level::trace)    { return "trace"; }
                else if constexpr (Level == level::debug)    { return "debug"; }
                else if constexpr (Level == level::info)     { return "info"; }
                else if constexpr (Level == level::warn)     { return "warn"; }
                else if constexpr (Level == level::error)    { return "error"; }
                else if constexpr (Level == level::critical) { return "critical"; }
                // clang-format on

                return "?";
            } // log_level_as_string

            template <typename ForwardIt>
            auto to_json_string(ForwardIt _first, ForwardIt _last) const -> std::string
            {
                using json = nlohmann::json;
                using container = std::unordered_map<std::string, std::string>;

                json object = container(_first, _last);

                object[tag::log::category] = logger_config<Category>::name;
                object[tag::log::level] = log_level_as_string();
                object[tag::log::facility] = "local0";

                if (const auto api_num = get_request_api_number(); api_num) {
                    object[tag::request::api_number] = *api_num;
                    auto iter = irods::api_number_names.find(*api_num);

                    if (std::end(irods::api_number_names) != iter) {
                        object[tag::request::api_name] = iter->second;
                    }
                    else {
                        object[tag::request::api_name] = "";
                    }
                }

                if (const auto* vers = get_request_client_version(); vers) {
                    object[tag::request::release_version] = vers->relVersion;
                    object[tag::request::api_version] = vers->apiVersion;
                }

                if (const auto hostname = get_request_client_hostname(); !hostname.empty()) {
                    object[tag::request::host] = hostname;
                }

                if (const auto username = get_request_client_username(); !username.empty()) {
                    object[tag::request::client_user] = username;
                }

                if (const auto username = get_request_proxy_username(); !username.empty()) {
                    object[tag::request::proxy_user] = username;
                }

                object[tag::server::type] = get_server_type();
                object[tag::server::host] = get_server_hostname();
                object[tag::server::pid] = getpid();
                object[tag::server::timestamp] = utc_timestamp();

                return object.dump();
            } // to_json_string

            template <
                typename ForwardIt,
                typename ValueType = typename std::iterator_traits<ForwardIt>::value_type,
                typename = std::enable_if_t<std::is_same_v<ValueType, log::key_value>>>
            constexpr auto log_message(ForwardIt _first, ForwardIt _last) const -> void
            {
                const auto msg = to_json_string(_first, _last);

                if constexpr (Level == level::trace) {
                    detail::get_logger()->trace(msg);
                }
                else if constexpr (Level == level::debug) {
                    detail::get_logger()->debug(msg);
                }
                else if constexpr (Level == level::info) {
                    detail::get_logger()->info(msg);
                }
                else if constexpr (Level == level::warn) {
                    detail::get_logger()->warn(msg);
                }
                else if constexpr (Level == level::error) {
                    detail::get_logger()->error(msg);
                }
                else if constexpr (Level == level::critical) {
                    detail::get_logger()->critical(msg);
                }

                append_to_r_error_stack(_first, _last);
            } // log_message

            template <typename ForwardIt>
            constexpr auto append_to_r_error_stack(ForwardIt _first, ForwardIt _last) const -> void
            {
                if (!should_write_to_error_object()) {
                    return;
                }

                if (auto* const error = get_error_object(); error) {
                    for (const auto& [k, v] : boost::make_iterator_range(_first, _last)) {
                        const auto msg = fmt::format("{}: {}", k, v);

                        // Force addRErrorMsg() to skip prefixing the message with 'Level N:'.
                        // When the output is printed on the client-side, it will be very close
                        // to the log file.
                        constexpr int disable_level_prefix = STDOUT_STATUS;
                        addRErrorMsg(error, disable_level_prefix, msg.c_str());
                    }
                }
            } // append_to_r_error_stack
#endif // IRODS_ENABLE_SYSLOG
        }; // class impl

        logger() = delete;

        logger(const logger&) = delete;
        auto operator=(const logger&) -> logger& = delete;

        /// Sets the log level for a specific logger.
        ///
        /// This member function is not thread-safe.
        ///
        /// \param[in] _level The log level that will be used by the logger.
        ///
        /// \since 4.3.0
        static constexpr auto set_level(level _level) noexcept -> void
        {
            logger_config<Category>::level = _level;
        } // set_level

        // clang-format off
        inline static const auto trace    = impl<level::trace>{};
        inline static const auto debug    = impl<level::debug>{};
        inline static const auto info     = impl<level::info>{};
        inline static const auto warn     = impl<level::warn>{};
        inline static const auto error    = impl<level::error>{};
        inline static const auto critical = impl<level::critical>{};
        // clang-format on
    }; // class logger

    //
    // Pre-defined Logger Category Configurations
    //

    template <>
    class logger_config<category::legacy>
    {
        static constexpr const char* const name = "legacy";
        inline static level level = level::info; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        friend class logger<category::legacy>;
    }; // class logger_config<category::legacy>

    template <>
    class logger_config<category::server>
    {
        static constexpr const char* const name = "server";
        inline static level level = level::info; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        friend class logger<category::server>;
    }; // class logger_config<category::server>

    template <>
    class logger_config<category::agent_factory>
    {
        static constexpr const char* const name = "agent_factory";
        inline static level level = level::info; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        friend class logger<category::agent_factory>;
    }; // class logger_config<category::agent_factory>

    template <>
    class logger_config<category::agent>
    {
        static constexpr const char* const name = "agent";
        inline static level level = level::info; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        friend class logger<category::agent>;
    }; // class logger_config<category::agent>

    template <>
    class logger_config<category::delay_server>
    {
        static constexpr const char* const name = "delay_server";
        inline static level level = level::info; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        friend class logger<category::delay_server>;
    }; // class logger_config<category::delay_server>

    template <>
    class logger_config<category::resource>
    {
        static constexpr const char* const name = "resource";
        inline static level level = level::info; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        friend class logger<category::resource>;
    }; // class logger_config<category::resource>

    template <>
    class logger_config<category::database>
    {
        static constexpr const char* const name = "database";
        inline static level level = level::info; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        friend class logger<category::database>;
    }; // class logger_config<category::database>

    template <>
    class logger_config<category::authentication>
    {
        static constexpr const char* const name = "authentication";
        inline static level level = level::info; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        friend class logger<category::authentication>;
    }; // class logger_config<category::authentication>

    template <>
    class logger_config<category::api>
    {
        static constexpr const char* const name = "api";
        inline static level level = level::info; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        friend class logger<category::api>;
    }; // class logger_config<category::api>

    template <>
    class logger_config<category::microservice>
    {
        static constexpr const char* const name = "microservice";
        inline static level level = level::info; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        friend class logger<category::microservice>;
    }; // class logger_config<category::microservice>

    template <>
    class logger_config<category::network>
    {
        static constexpr const char* const name = "network";
        inline static level level = level::info; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        friend class logger<category::network>;
    }; // class logger_config<category::network>

    template <>
    class logger_config<category::rule_engine>
    {
        static constexpr const char* const name = "rule_engine";
        inline static level level = level::info; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        friend class logger<category::rule_engine>;
    }; // class logger_config<category::rule_engine>

    template <>
    class logger_config<category::sql>
    {
        static constexpr const char* const name = "sql";
        inline static level level = level::info; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        friend class logger<category::sql>;
    }; // class logger_config<category::sql>
} // namespace irods::experimental::log

#endif // IRODS_LOGGER_HPP

