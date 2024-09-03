#include "irods/irods_signal.hpp"

#include "irods/irods_default_paths.hpp"
#include "irods/irods_exception.hpp"
#include "irods/irods_logger.hpp"
#include "irods/rodsErrorTable.h"

#include <boost/stacktrace/safe_dump_to.hpp>

#include <array>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <system_error>
#include <charconv>
#include <string>

#include <unistd.h>

namespace
{
    // This value represents the maximum size of a path for stacktrace handling. By picking a
    // value, we are opting to "try" writing the stacktrace to a file first, rather than checking
    // the OS limits and adapting to them.
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    constexpr auto g_max_path_size = 1024;

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    std::array<char, g_max_path_size> g_stacktrace_dir;
} // anonymous namespace

extern "C"
void stacktrace_signal_handler(int _signal)
{
    timespec ts; // NOLINT(cppcoreguidelines-pro-type-member-init)
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        _exit(_signal);
    }

    char file_path_unprocessed[g_max_path_size];
    std::strncpy(file_path_unprocessed, g_stacktrace_dir.data(), g_max_path_size);
    std::strcat(file_path_unprocessed, "/");

    // At this point, "file_path_unprocessed" should have the following path:
    //
    //     <irods_home>/stacktraces/
    //

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* buf_start = file_path_unprocessed + strlen(file_path_unprocessed);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto* buf_end = file_path_unprocessed + sizeof(file_path_unprocessed);

    if (auto [sec_end, ec] = std::to_chars(buf_start, buf_end, ts.tv_sec); std::errc{} == ec) {
        *sec_end = '.';

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (auto [msec_end, ec] = std::to_chars(sec_end + 1, buf_end, ts.tv_nsec / 1'000'000);
            std::errc{} == ec)
        {
            *msec_end = '.';

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (auto [pid_end, ec] = std::to_chars(msec_end + 1, buf_end, getpid()); std::errc{} == ec) {
                // Terminate the path string. The filename should have the following format:
                //
                //     <epoch_seconds>.<epoch_milliseconds>.<agent_pid>
                //
                // This keeps the files sorted by timestamp and by pid.
                *pid_end = 0;

                char file_path[g_max_path_size];
                std::strcpy(file_path, file_path_unprocessed);
                
                // An indicator to the thread handling the logging of these files that this file
                // is not ready to be logged (i.e. the signal handler is still writing to it).
                std::strcat(file_path_unprocessed, irods::STACKTRACE_NOT_READY_FOR_LOGGING_SUFFIX);

                boost::stacktrace::safe_dump_to(file_path_unprocessed);

                // Now the stacktrace has been dumped, rename the file so that the trailing
                // suffix (i.e. ".not_ready_for_logging") is removed.
                std::rename(file_path_unprocessed, file_path);
            }
        }
    }

    _exit(_signal);
} // stacktrace_signal_handler

namespace irods
{
    const char* const STACKTRACE_NOT_READY_FOR_LOGGING_SUFFIX = ".not_ready_for_logging";

    auto get_stacktrace_directory_path() -> std::string_view
    {
        return g_stacktrace_dir.data();
    } // get_stacktrace_directory_path

    auto setup_unrecoverable_signal_handlers() -> void
    {
        using log_server = irods::experimental::log::server;

        std::fill(std::begin(g_stacktrace_dir), std::end(g_stacktrace_dir), 0);
        const auto path = get_irods_stacktrace_directory().string();
        if (path.size() >= g_stacktrace_dir.size()) {
            constexpr const auto* msg = "Stacktrace directory path cannot exceed buffer size [{}]";
            THROW(CONFIGURATION_ERROR, fmt::format(msg, g_stacktrace_dir.size()));
        }
        path.copy(g_stacktrace_dir.data(), g_stacktrace_dir.size());
        log_server::debug("{}: Stacktraces will be dumped to [{}].", __func__, g_stacktrace_dir.data());

        log_server::debug("{}: Setting up unrecoverable stacktrace signal handlers.", __func__);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        struct sigaction action;
        action.sa_flags = 0;
        sigemptyset(&action.sa_mask);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        action.sa_handler = stacktrace_signal_handler;

        constexpr const char* env_var = "IRODS_DISABLE_CRASH_SIGNAL_HANDLERS";
        const char *env_var_val = std::getenv(env_var); // NOLINT(concurrency-mt-unsafe)
        if (nullptr == env_var_val || (std::strncmp(env_var_val, "0", sizeof("0")) == 0)) {
            sigaction(SIGSEGV, &action, nullptr);
            sigaction(SIGABRT, &action, nullptr);
            sigaction(SIGILL, &action, nullptr);
            sigaction(SIGFPE, &action, nullptr);
            sigaction(SIGBUS, &action, nullptr);
        }
        else {
            log_server::warn("{}: {} is set; crash signal handlers disabled.", __func__, env_var);
        }
    } // set_unrecoverable_signal_handlers
} // namespace irods
