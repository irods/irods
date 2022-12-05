#include "irods/irods_signal.hpp"

#include "irods/irods_default_paths.hpp"
#include "irods/irods_logger.hpp"

#include <boost/stacktrace/safe_dump_to.hpp>

#include <climits> // For _POSIX_PATH_MAX
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <system_error>
#include <charconv>
#include <string>

#include <unistd.h>

extern "C"
void stacktrace_signal_handler(int _signal)
{
    timespec ts;

    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        _exit(_signal);
    }

    // "_POSIX_PATH_MAX" is guaranteed to be defined as 256.
    // This value represents the minimum path length supported by POSIX operating systems.
    char file_path_unprocessed[_POSIX_PATH_MAX];
    std::strncpy(file_path_unprocessed, irods::STACKTRACE_DIR.c_str(), _POSIX_PATH_MAX);
    std::strcat(file_path_unprocessed, "/");
    // At this point, "file_path_unprocessed" should have the following path:
    //
    //     <irods_home>/stacktraces/
    //

    auto* buf_start = file_path_unprocessed + strlen(file_path_unprocessed);
    auto* buf_end = file_path_unprocessed + sizeof(file_path_unprocessed);

    if (auto [sec_end, ec] = std::to_chars(buf_start, buf_end, ts.tv_sec); std::errc{} == ec) {
        *sec_end = '.';

        if (auto [msec_end, ec] = std::to_chars(sec_end + 1, buf_end, ts.tv_nsec / 1'000'000);
            std::errc{} == ec)
        {
            *msec_end = '.';

            if (auto [pid_end, ec] = std::to_chars(msec_end + 1, buf_end, getpid()); std::errc{} == ec) {
                // Terminate the path string. The filename should have the following format:
                //
                //     <epoch_seconds>.<epoch_milliseconds>.<agent_pid>
                //
                // This keeps the files sorted by timestamp and by pid.
                *pid_end = 0;

                char file_path[_POSIX_PATH_MAX];
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

    const std::string STACKTRACE_DIR(irods::get_irods_stacktrace_directory().string());

    void set_unrecoverable_signal_handlers()
    {
        using log_server = irods::experimental::log::server;

        log_server::debug("Setting stacktrace dump signal handler for process [{}].", getpid());
        log_server::debug("Stacktraces will be dumped to [{}].", irods::STACKTRACE_DIR);

        struct sigaction action;
        action.sa_flags = 0;
        sigemptyset(&action.sa_mask);
        action.sa_handler = stacktrace_signal_handler;

        sigaction(SIGSEGV, &action, nullptr);
        sigaction(SIGABRT, &action, nullptr);
        sigaction(SIGINT, &action, nullptr);
    } // set_unrecoverable_signal_handlers
} // namespace irods
