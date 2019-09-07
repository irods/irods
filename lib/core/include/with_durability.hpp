#ifndef IRODS_WITH_DURABILITY_HPP
#define IRODS_WITH_DURABILITY_HPP

#include <iostream>
#include <chrono>
#include <thread>

namespace irods::experimental
{
    /// A type that specifies how many times \p with_durability should
    /// retry on failure of a function-like object.
    struct retries { int value; };

    /// A type that specifies how long (in milliseconds) \p with_durability
    /// should wait before attempting to invoke a function-like object.
    struct delay { std::chrono::milliseconds value; };

    /// A type that specifies a multiplier of \p delay.
    struct delay_multiplier { float value; };

    /// TODO Documentation
    /// Options that control the behavior of \p with_durability.
    struct durability_options
    {
        template <typename ...Args>
        durability_options(Args&&... args)
            : retries{2}
            , delay{1}
            , delay_multiplier{1.f}
        {
            (set(std::forward<Args>(args)), ...);
        }

        int retries;
        std::chrono::milliseconds delay;
        float delay_multiplier;

        template <typename Function>
        friend void with_durability(const durability_options, Function) noexcept;

    private:
        void set(struct retries v)
        {
            throw_if_less_than_zero(v.value);
            retries = v.value;
        }

        void set(struct delay v)
        {
            throw_if_less_than_zero(v.value.count());
            delay = v.value;
        }

        void set(struct delay_multiplier v)
        {
            throw_if_less_than_zero(v.value);
            delay_multiplier = v.value;
        }

        template <typename T>
        void throw_if_less_than_zero(T v)
        {
            if (v < 0) {
                throw std::runtime_error{"Value must be greater than or equal to zero."};
            }
        }
    }; // struct durability_options

    /// An enumeration used to indicate how \p with_durability should proceed. These values
    /// are also used to inform the caller of \p with_durability of what happened.
    enum class execution_result
    {
        ready,      ///< The state before invoking the callable.
        success,    ///< The callable succeeded.
        failure,    ///< The callable failed.
        exception,  ///< The callable leaked an exception.
        stop        ///< The callable manually terminated execution.
    };

    /// Invokes a function or function-like object multiple times based on specific requirements.
    ///
    /// 
    ///
    /// \user Client
    /// \user Server
    ///
    /// \since 4.2.8
    ///
    /// \param[in] opts The options that control how \p func is handled.
    /// \param[in] func The function or function-like object to invoke.
    ///
    /// return execution_result            An indicator of what happened.
    /// retval execution_result::ready     Indicates that \p func never ran.
    /// retval execution_result::success   Indicates that \p func was successful.
    /// retval execution_result::failure   Indicates that \p func failed.
    /// retval execution_result::exception Indicates that \p func failed.
    /// retval execution_result::stop      Indicates that \p func was stopped manually.
    template <typename Function>
    auto with_durability(const durability_options& opts, Function func) noexcept -> execution_result
    {
        auto exec_result = execution_result::ready;
        auto temp_delay = opts.delay;

        for (int i = 0; i < opts.retries; ++i) {
            try {
                exec_result = func();

                if (exec_result != execution_result::failure) {
                    return exec_result;
                }

                std::this_thread::sleep_for(temp_delay);
                temp_delay *= opts.delay_multiplier;
            }
            catch (...) {
                exec_result = execution_result::exception;
            }
        }

        return exec_result;
    }

    namespace translator
    {
        inline auto equals_zero(int error_code) -> execution_result
        {
            return error_code == 0 ? execution_result::success : execution_result::failure;
        }

        inline auto greater_than_or_equal_to_zero(int error_code) -> execution_result
        {
            return error_code < 0 ? execution_result::failure : execution_result::success;
        }
    } // namespace translator
} // namespace irods::experimental

#endif // IRODS_WITH_DURABILITY_HPP

