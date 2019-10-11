//
// Logger Configuration
//

template <>
class log::logger_config<log::category::legacy>
{
    static constexpr const char* name = "legacy";
    inline static log::level level = log::level::info;

    friend class logger<log::category::legacy>;
};

template <>
class log::logger_config<log::category::server>
{
    static constexpr const char* name = "server";
    inline static log::level level = log::level::info;

    friend class logger<log::category::server>;
};

template <>
class log::logger_config<log::category::agent_factory>
{
    static constexpr const char* name = "agent_factory";
    inline static log::level level = log::level::info;

    friend class logger<log::category::agent_factory>;
};

template <>
class log::logger_config<log::category::agent>
{
    static constexpr const char* name = "agent";
    inline static log::level level = log::level::info;

    friend class logger<log::category::agent>;
};

template <>
class log::logger_config<log::category::delay_server>
{
    static constexpr const char* name = "delay_server";
    inline static log::level level = log::level::info;

    friend class logger<log::category::delay_server>;
};

template <>
class log::logger_config<log::category::resource>
{
    static constexpr const char* name = "resource";
    inline static log::level level = log::level::info;

    friend class logger<log::category::resource>;
};

template <>
class log::logger_config<log::category::database>
{
    static constexpr const char* name = "database";
    inline static log::level level = log::level::info;

    friend class logger<log::category::database>;
};

template <>
class log::logger_config<log::category::authentication>
{
    static constexpr const char* name = "authentication";
    inline static log::level level = log::level::info;

    friend class logger<log::category::authentication>;
};

template <>
class log::logger_config<log::category::api>
{
    static constexpr const char* name = "api";
    inline static log::level level = log::level::info;

    friend class logger<log::category::api>;
};

template <>
class log::logger_config<log::category::microservice>
{
    static constexpr const char* name = "microservice";
    inline static log::level level = log::level::info;

    friend class logger<log::category::microservice>;
};

template <>
class log::logger_config<log::category::network>
{
    static constexpr const char* name = "network";
    inline static log::level level = log::level::info;

    friend class logger<log::category::network>;
};

template <>
class log::logger_config<log::category::rule_engine>
{
    static constexpr const char* name = "rule_engine";
    inline static log::level level = log::level::info;

    friend class logger<log::category::rule_engine>;
};

//
// Logger
//

template <typename Category>
void log::logger<Category>::set_level(log::level _level) noexcept
{
    logger_config<Category>::level = _level;
}

template <typename Category>
template <log::level Level>
class log::logger<Category>::impl
{
public:
    template <typename T>
    using is_iterable = decltype(std::begin(std::declval<std::decay_t<T>>()));

    void operator()(const std::string& _msg) const
    {
        (*this)({{tag::log::message, _msg}});
    }

    void operator()(std::initializer_list<log::key_value> _list) const noexcept
    {
        (*this)(std::begin(_list), std::end(_list));
    }

    template <typename Container,
              typename = is_iterable<Container>>
    void operator()(const Container& _container) const noexcept
    {
        (*this)(std::begin(_container), std::end(_container));
    }

    template <typename ForwardIt>
    void operator()(ForwardIt _first, ForwardIt _last) const noexcept
    {
        if (!should_log()) {
            return;
        }

        const auto msg = to_json_string(_first, _last);

        if constexpr (Level == level::trace) {
            log_->trace(msg);
        }
        else if constexpr (Level == level::debug) {
            log_->debug(msg);
        }
        else if constexpr (Level == level::info) {
            log_->info(msg);
        }
        else if constexpr (Level == level::warn) {
            log_->warn(msg);
        }
        else if constexpr (Level == level::error) {
            log_->error(msg);
        }
        else if constexpr (Level == level::critical) {
            log_->critical(msg);
        }

        append_to_r_error_stack(_first, _last);
    }

private:
    struct tag
    {
        struct log
        {
            // clang-format off
            inline static const char* category        = "log_category";
            inline static const char* facility        = "log_facility";
            inline static const char* message         = "log_message";
            inline static const char* level           = "log_level";
            // clang-format on
        };

        struct request
        {
            // clang-format off
            inline static const char* release_version = "request_release_version";
            inline static const char* api_version     = "request_api_version";
            inline static const char* host            = "request_host";
            inline static const char* client_user     = "request_client_user";
            inline static const char* proxy_user      = "request_proxy_user";
            inline static const char* api_number      = "request_api_number";
            inline static const char* api_name        = "request_api_name";
            // clang-format on
        };

        struct server
        {
            // clang-format off
            inline static const char* type            = "server_type";
            inline static const char* host            = "server_host";
            inline static const char* pid             = "server_pid";
            inline static const char* name            = "server_name";
            inline static const char* timestamp       = "server_timestamp";
            // clang-format on
        };
    };

    impl() = default;

    impl(const impl&) = delete;
    impl& operator=(const impl&) = delete;

    bool should_log() const noexcept
    {
        return Level >= logger_config<Category>::level;
    }

    std::string utc_timestamp() const
    {
        // clang-format off
        using clock      = std::chrono::system_clock;
        using time_point = std::chrono::time_point<clock>;
        // clang-format on

        timeval tv{};

        if (auto ec = gettimeofday(&tv, nullptr); ec != 0) {
            auto now = clock::to_time_t(clock::now());

            std::stringstream ss;
            ss << std::put_time(std::gmtime(&now), "%FT%T.0");

            return ss.str();
        }

        auto now = clock::to_time_t(time_point{std::chrono::seconds{tv.tv_sec}});

        std::stringstream ss;
        ss << std::put_time(std::gmtime(&now), "%FT%T.")
           << std::setw(6) << std::setfill('0') << std::left
           << tv.tv_usec;

        return ss.str();
    }

    static constexpr const char* log_level_as_string() noexcept
    {
        // clang-format off
        if constexpr (Level == level::trace)    { return "trace"; }
        if constexpr (Level == level::debug)    { return "debug"; }
        if constexpr (Level == level::info)     { return "info"; }
        if constexpr (Level == level::warn)     { return "warn"; }
        if constexpr (Level == level::error)    { return "error"; }
        if constexpr (Level == level::critical) { return "critical"; }
        // clang-format on

        return "?";
    }

    template <typename ForwardIt>
    std::string to_json_string(ForwardIt _first, ForwardIt _last) const
    {
        using json = nlohmann::json;
        using container = std::unordered_map<std::string, std::string>;

        json object = container(_first, _last);

        object[tag::log::category] = logger_config<Category>::name;
        object[tag::log::level] = log_level_as_string();
        object[tag::log::facility] = "local0";

        if (log_api_number_) {
            object[tag::request::api_number] = api_number_;

            if (auto iter = irods::api_number_names.find(api_number_);
                std::end(irods::api_number_names) != iter)
            {
                object[tag::request::api_name] = iter->second;
            }
            else {
                object[tag::request::api_name] = "";
            }
        }

        if (req_client_version_) {
            object[tag::request::release_version] = req_client_version_->relVersion;
            object[tag::request::api_version] = req_client_version_->apiVersion;
        }

        if (!req_client_host_.empty()) {
            object[tag::request::host] = req_client_host_;
        }

        if (!req_client_user_.empty()) {
            object[tag::request::client_user] = req_client_user_;
        }

        if (!req_proxy_user_.empty()) {
            object[tag::request::proxy_user] = req_proxy_user_;
        }

        object[tag::server::type] = server_type_;
        object[tag::server::host] = server_host_;
        object[tag::server::pid] = getpid();
        object[tag::server::timestamp] = utc_timestamp();

        return object.dump();
    }

    template <typename ForwardIt>
    void append_to_r_error_stack(ForwardIt _first, ForwardIt _last) const
    {
        if (error_ && write_to_error_object_) {
            for (const auto& [k, v] : boost::make_iterator_range(_first, _last)) {
                std::string msg = k;
                msg += ": ";
                msg += v;

                // Force addRErrorMsg() to skip prefixing the message with 'Level N:'.
                // When the output is printed on the client-side, it will be very close
                // to the log file.
                constexpr int disable_level_prefix = STDOUT_STATUS;

                if (const auto ec = addRErrorMsg(error_, disable_level_prefix, msg.c_str()); ec) {
                    logger<Category>::impl<level::error>{}({
                        {tag::log::message, "Failed to append message to rError stack"},
                        {"error_code", std::to_string(ec)}
                    });
                }
            }
        }
    }
};

