#ifndef IRODS_WITH_DURABILITY_HPP
#define IRODS_WITH_DURABILITY_HPP

/// \file

#include <chrono>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

namespace irods::experimental
{
    /// A type that specifies how many times \p with_durability should
    /// retry on failure of a function-like object.
    struct retries
    {
        explicit constexpr retries(int v) noexcept
            : value{v}
        {
        }

        const int value;
    };

    /// A type that specifies how long (in milliseconds) \p with_durability
    /// should wait before attempting to invoke a function-like object.
    struct delay
    {
        explicit constexpr delay(std::chrono::milliseconds v) noexcept
            : value{v}
        {
        }

        const std::chrono::milliseconds value;
    };

    /// A type that specifies a multiplier of \p delay.
    struct delay_multiplier
    {
        explicit constexpr delay_multiplier(float v) noexcept
            : value{v}
        {
        }

        const float value;
    };

    /// A type that holds options used to control the behavior of \p with_durability.
    class durability_options
    {
    private:
        int retries_;
        std::chrono::milliseconds delay_;
        float delay_multiplier_;

        template <typename T>
        constexpr auto throw_if_less_than_zero(T v) -> void
        {
            if (v < 0) {
                throw std::invalid_argument{"Value must be greater than or equal to zero."};
            }
        }

        constexpr auto set(struct retries v) -> void
        {
            throw_if_less_than_zero(v.value);
            retries_ = v.value;
        }

        constexpr auto set(struct delay v) -> void
        {
            throw_if_less_than_zero(v.value.count());
            delay_ = v.value;
        }

        constexpr auto set(struct delay_multiplier v) -> void
        {
            throw_if_less_than_zero(v.value);
            delay_multiplier_ = v.value;
        }

        // Fallback case. Ignores unknown arguments.
        template <typename T>
        constexpr auto set(T&&) const noexcept -> void
        {
        }

    public:
        constexpr durability_options() noexcept
            : retries_{1}
            , delay_{1000}
            , delay_multiplier_{1.f}
        {
        }

        explicit constexpr durability_options(retries other)
            : durability_options{}
        {
            set(other);
        }

        explicit constexpr durability_options(delay other)
            : durability_options{}
        {
            set(other);
        }

        explicit constexpr durability_options(delay_multiplier other)
            : durability_options{}
        {
            set(other);
        }

        template <typename ...Args>
        constexpr durability_options(Args&&... args)
            : durability_options{}
        {
            (set(std::forward<Args>(args)), ...);
        }

        constexpr auto retries() const noexcept -> int
        {
            return retries_;
        }

        constexpr auto delay() const noexcept -> std::chrono::milliseconds
        {
            return delay_;
        }

        constexpr auto delay_multiplier() const noexcept -> float
        {
            return delay_multiplier_;
        }

        auto set_retries(int v)
        {
            throw_if_less_than_zero(v);
            retries_ = v;
        }

        auto set_delay(std::chrono::milliseconds v)
        {
            throw_if_less_than_zero(v.count());
            delay_ = v;
        }

        auto set_delay_multiplier(float v)
        {
            throw_if_less_than_zero(v);
            delay_multiplier_ = v;
        }
    }; // class durability_options

    namespace detail
    {
        template <typename ...Ts>
        struct last_type
        {
            template <typename T> struct tag { using type = T; };
            using type = typename decltype((tag<Ts>{}, ...))::type;
        };

        template <typename ...Ts>
        using last_type_t = typename last_type<Ts...>::type;

        template <typename T>
        constexpr auto last_arg(T&& t) noexcept
        {
            return std::forward<T>(t);
        }

        template <typename T, typename ...Ts>
        constexpr auto last_arg(T&&, Ts&&... ts) noexcept
        {
            return last_arg(std::forward<Ts>(ts)...);
        }
    } // namespace detail

    /// An enumeration used to indicate how \p with_durability should proceed. These values
    /// are also used to inform the caller of \p with_durability of what happened.
    enum class execution_result
    {
        ready,      ///< The state before invoking the callable.
        success,    ///< The callable succeeded.
        failure,    ///< The callable failed.
        exception,  ///< The callable leaked an exception.
        stop        ///< The callable manually terminated execution.
    }; // enum class execution_result

    /// A convenient retry mechanism for functions and function-like objects.
    ///
    /// Takes a set of options defining at most how many times a given function-like object
    /// will be invoked (retried) following a failure. The function-like object is always invoked
    /// at least once. It will be invoked once more for each failure until the specified retry
    /// conditions are met or it succeeds or it is stopped manually.
    ///
    /// \since 4.2.8
    ///
    /// \param[in] opts The options that control how \p func is handled.
    /// \param[in] func The function or function-like object to invoke.
    ///
    /// \return An \p execution_result.
    /// \retval execution_result::ready     Indicates that \p func was never invoked.
    /// \retval execution_result::success   Indicates that \p func was successful.
    /// \retval execution_result::failure   Indicates that \p func failed.
    /// \retval execution_result::exception Indicates that \p func failed due to an exception.
    /// \retval execution_result::stop      Indicates that \p func was stopped manually.
    template <typename Function>
    auto with_durability(const durability_options& opts, Function func) noexcept -> execution_result
    {
        const auto iterations = opts.retries() + 1;
        auto temp_delay = opts.delay();
        auto exec_result = execution_result::ready;

        for (int i = 0; i < iterations; ++i) {
            try {
                exec_result = func();

                if (exec_result == execution_result::success ||
                    exec_result == execution_result::stop)
                {
                    return exec_result;
                }

                std::this_thread::sleep_for(temp_delay);
                temp_delay *= opts.delay_multiplier();
            }
            catch (...) {
                exec_result = execution_result::exception;
            }
        }

        return exec_result;
    }

    /// A convenient retry mechanism for functions and function-like objects.
    ///
    /// \since 4.2.8
    ///
    /// \param[in] args A parameter pack optionally containing \p objects used to construct an
    ///                 instance of \p durability_options followed by a function or function-like
    ///                 object to invoke. The last argument must be a function-like object.
    ///
    /// \return An \p execution_result.
    /// \retval execution_result::ready     Indicates that the function-like object was never invoked.
    /// \retval execution_result::success   Indicates that the function-like object was successful.
    /// \retval execution_result::failure   Indicates that the function-like object failed.
    /// \retval execution_result::exception Indicates that the function-like object failed due to an exception.
    /// \retval execution_result::stop      Indicates that the function-like object was stopped manually.
    template <typename ...Args>
    auto with_durability(Args&& ...args) noexcept -> execution_result
    {
        using function_type = detail::last_type_t<Args...>;
        static_assert(std::is_invocable_v<function_type>);
        return with_durability<function_type>({std::forward<Args>(args)...}, detail::last_arg(std::forward<Args>(args)...));
    }

    /// A namespace containing functions that provide the most common translations
    /// from integer error codes and other results to an \p execution_result.
    namespace translator
    {
        /// Returns an \p execution_result based on whether \p error_code is equal to zero.
        ///
        /// \since 4.2.8
        ///
        /// \param[in] error_code The integer value to check.
        ///
        /// \return An \p execution_result.
        /// \retval execution_result::success If \p error_code is equal to zero.
        /// \retval execution_result::failure If \p error_code is not equal to zero.
        inline constexpr auto equals_zero(int error_code) noexcept -> execution_result
        {
            return error_code != 0 ? execution_result::failure : execution_result::success;
        }

        /// Returns an \p execution_result based on whether \p error_code is greater than
        /// or equal to zero.
        ///
        /// \since 4.2.8
        ///
        /// \param[in] error_code The integer value to check.
        ///
        /// \return An \p execution_result.
        /// \retval execution_result::success If \p error_code is greater than or equal to zero.
        /// \retval execution_result::failure If \p error_code is less than zero.
        inline constexpr auto greater_than_or_equal_to_zero(int error_code) noexcept -> execution_result
        {
            return error_code < 0 ? execution_result::failure : execution_result::success;
        }
    } // namespace translator
} // namespace irods::experimental

#endif // IRODS_WITH_DURABILITY_HPP

