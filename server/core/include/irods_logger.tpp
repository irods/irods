//
// Logger Configuration
//

template <>
class log::logger_config<log::category::legacy>
{
    static constexpr char* name = "legacy";
    inline static log::level level = log::level::info;

    friend class logger<log::category::legacy>;
};

template <>
class log::logger_config<log::category::server>
{
    static constexpr char* name = "server";
    inline static log::level level = log::level::info;

    friend class logger<log::category::server>;
};

template <>
class log::logger_config<log::category::agent>
{
    static constexpr char* name = "agent";
    inline static log::level level = log::level::info;

    friend class logger<log::category::agent>;
};

template <>
class log::logger_config<log::category::resource>
{
    static constexpr char* name = "resource";
    inline static log::level level = log::level::info;

    friend class logger<log::category::resource>;
};

template <>
class log::logger_config<log::category::database>
{
    static constexpr char* name = "database";
    inline static log::level level = log::level::info;

    friend class logger<log::category::database>;
};

template <>
class log::logger_config<log::category::auth>
{
    static constexpr char* name = "auth";
    inline static log::level level = log::level::info;

    friend class logger<log::category::auth>;
};

template <>
class log::logger_config<log::category::api>
{
    static constexpr char* name = "api";
    inline static log::level level = log::level::info;

    friend class logger<log::category::api>;
};

template <>
class log::logger_config<log::category::msi>
{
    static constexpr char* name = "msi";
    inline static log::level level = log::level::info;

    friend class logger<log::category::msi>;
};

template <>
class log::logger_config<log::category::network>
{
    static constexpr char* name = "network";
    inline static log::level level = log::level::info;

    friend class logger<log::category::network>;
};

template <>
class log::logger_config<log::category::rule_engine>
{
    static constexpr char* name = "rule_engine";
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
        (*this)({{"msg", _msg}});
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
        if (!should_log())
        {
            return;
        }

        const auto msg = to_json_string(logger_config<Category>::name, _first, _last);

        if constexpr (Level == level::trace)
        {
            log_->trace(msg);
        }
        else if constexpr (Level == level::debug)
        {
            log_->debug(msg);
        }
        else if constexpr (Level == level::info)
        {
            log_->info(msg);
        }
        else if constexpr (Level == level::warn)
        {
            log_->warn(msg);
        }
        else if constexpr (Level == level::error)
        {
            log_->error(msg);
        }
        else if constexpr (Level == level::critical)
        {
            log_->critical(msg);
        }

        append_to_r_error_stack(_first, _last);
    }

private:
    impl() = default;

    impl(const impl&) = delete;
    impl& operator=(const impl&) = delete;

    bool should_log() const noexcept
    {
        return Level >= logger_config<Category>::level;
    }

    template <typename ForwardIt>
    std::string to_json_string(const std::string& _src, ForwardIt _first, ForwardIt _last) const
    {
        using json = nlohmann::json;
        using container = std::unordered_map<std::string, std::string>;

        container data(_first, _last);
        data["src"] = _src;

        return json(data).dump();
    }

    template <typename ForwardIt>
    void append_to_r_error_stack(ForwardIt _first, ForwardIt _last) const
    {
        if (error_ && write_to_error_object_)
        {
            for (const auto& [k, v] : boost::make_iterator_range(_first, _last))
            {
                std::string msg = k;
                msg += ": ";
                msg += v;

                // Force addRErrorMsg() to skip prefixing the message with 'Level N:'.
                // When the output is printed on the client-side, it will be very close
                // to the log file.
                constexpr int disable_level_prefix = STDOUT_STATUS;

                if (const auto ec = addRErrorMsg(error_, disable_level_prefix, msg.c_str()); ec)
                {
                    logger<Category>::impl<level::error>{}({
                        {"msg", "Failed to append message to rError stack"},
                        {"code", std::to_string(ec)}
                    });
                }
            }
        }
    }
};

