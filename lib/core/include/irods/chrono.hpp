#ifndef IRODS_CHRONO_HPP
#define IRODS_CHRONO_HPP

/// \file

#include <fmt/format.h>
#include <fmt/compile.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <string>

#ifndef IRODS_CHRONO_FORMATTER_FMT
#  if FMT_VERSION >= 100000
#    define IRODS_CHRONO_FORMATTER_FMT 1
#  else
#    define IRODS_CHRONO_FORMATTER_FMT 0
#  endif
#endif

#if IRODS_CHRONO_FORMATTER_FMT
#  include <fmt/chrono.h>
#endif

#ifndef IRODS_STDLIB_CHRONO_HAS_CLOCK_STEADY
#  if defined(_GLIBCXX_USE_CLOCK_MONOTONIC) || (defined(_LIBCPP_VERSION) && !defined(_LIBCPP_HAS_NO_MONOTONIC_CLOCK))
#    define IRODS_STDLIB_CHRONO_HAS_CLOCK_STEADY
#  endif
#endif

#ifndef IRODS_STDLIB_CHRONO_HAS_CLOCK_STEADY
#  include <boost/chrono.hpp>
#endif

namespace irods
{
    namespace experimental::log
    {
        /// \brief Gets a UTC timestamp string suitable for logging.
        ///
        /// \return A string containing an ISO 8601 date and time string with millisecond precision.
        ///
        /// \since 5.0.0
        [[nodiscard]] static inline std::string utc_timestamp()
        {
            namespace chrono = std::chrono;
            using clock_type = chrono::system_clock;

#if IRODS_CHRONO_FORMATTER_FMT
            const auto tp = chrono::floor<chrono::milliseconds>(clock_type::now());
            return fmt::format(FMT_COMPILE("{:%FT%H:%M:%S}Z"), tp);
#else
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays, cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
            char ts_buf[64]{}; // 64 here isn't particularly special, just a large-enough small power-of-two
            std::timespec ts; // NOLINT(cppcoreguidelines-pro-type-member-init)
            if (const int base = std::timespec_get(&ts, TIME_UTC); base != TIME_UTC) {
                if (const int ec = clock_gettime(CLOCK_REALTIME, &ts); ec != 0) {
                    const auto tp = clock_type::now();
                    const auto tp_s = chrono::floor<chrono::seconds>(tp);
                    const auto tp_s_ms = chrono::floor<chrono::milliseconds>(tp_s);
                    const auto tp_ms = chrono::floor<chrono::milliseconds>(tp) - tp_s_ms;
                    const auto tt_s = clock_type::to_time_t(tp_s);

                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay, concurrency-mt-unsafe)
                    std::strftime(ts_buf, sizeof(ts_buf) - 1, "%FT%T", std::gmtime(&tt_s));
                    return fmt::format(FMT_COMPILE("{}.{:0>3}Z"), ts_buf, tp_ms.count());
                }
            }

            const std::time_t tt_s = static_cast<std::time_t>(ts.tv_sec);
            const auto ts_ms = ts.tv_nsec / 1000000L;

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay, concurrency-mt-unsafe)
            std::strftime(ts_buf, sizeof(ts_buf) - 1, "%FT%T", std::gmtime(&tt_s));
            return fmt::format(FMT_COMPILE("{}.{:0>3}Z"), ts_buf, ts_ms);
#endif
        } // utc_timestamp

    } //namespace experimental::log

    /// \brief Gets monotonic time in microseconds.
    ///
    /// This function gets the monotonic time in microseconds. Monotonic time is guaranteed to never go backwards and
    /// ignores changes to the system clock.
    ///
    /// systemd requires that certain service manager status notification messages contain a timestamp in this form.
    ///
    /// \return Unsigned integer containing current monotonic time in microseconds.
    [[nodiscard]] static inline std::uint64_t get_monotonic_usec()
    {
        std::timespec ts; // NOLINT(cppcoreguidelines-pro-type-member-init)
        int ec = clock_gettime(CLOCK_MONOTONIC, &ts);
#if defined(IRODS_STDLIB_CHRONO_HAS_CLOCK_STEADY)
        if (ec != 0) {
            namespace chrono = std::chrono;
            using clock_type = chrono::steady_clock;

            const auto tp = clock_type::now();
            const auto tp_usec = chrono::floor<chrono::microseconds>(tp);
            const auto td_usec = tp_usec.time_since_epoch();
            const auto b_usec = td_usec.count();
            return static_cast<std::uint64_t>(b_usec);
        }
#elif defined(BOOST_CHRONO_HAS_CLOCK_STEADY)
        if (ec != 0) {
            namespace chrono = boost::chrono;
            using clock_type = chrono::steady_clock;

            const auto tp = clock_type::now();
            const auto td = tp.time_since_epoch();
            const auto td_usec = chrono::floor<chrono::microseconds>(td);
            const auto b_usec = td_usec.count();
            return static_cast<std::uint64_t>(b_usec);
        }
#endif
        const std::uint64_t mt_s = static_cast<std::uint64_t>(ts.tv_sec);
        const std::uint64_t mt_ns = static_cast<std::uint64_t>(ts.tv_nsec);
        const std::uint64_t usec = (mt_s * 1000000) + (mt_ns / 1000);
        return usec;
    } // get_monotonic_usec
} //namespace irods

#endif // IRODS_CHRONO_HPP
