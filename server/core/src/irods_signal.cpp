#include "irods/irods_signal.hpp"

#include "irods/irods_default_paths.hpp"

#include <boost/stacktrace/safe_dump_to.hpp>

#include <climits> // For _POSIX_PATH_MAX
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <system_error>
#include <charconv>

#include <unistd.h>

extern "C"
void stacktrace_signal_handler(int _signal)
{
    timespec ts;

    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        std::exit(_signal);
    }

    // "_POSIX_PATH_MAX" is guaranteed to be defined as 256.
    // This value represents the minimum path length supported by POSIX operating systems.
    // Given that we're dealing with a relative path, a buffer of this size is acceptable here.
    char file_path_unprocessed[_POSIX_PATH_MAX] = "../../";
    std::strcat(file_path_unprocessed, IRODS_DEFAULT_PATH_HOMEDIR);
    std::strcat(file_path_unprocessed, "/stacktraces/");

    // At this point, "file_path_unprocessed" should have the following path:
    //
    //     ../../<irods_home>/stacktraces/
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

    std::exit(_signal);
} // stacktrace_signal_handler

namespace irods
{
    const char* const STACKTRACE_NOT_READY_FOR_LOGGING_SUFFIX = ".not_ready_for_logging";

    void set_unrecoverable_signal_handlers() // TODO Consider calling this for grandpa also.
    {
        struct sigaction action{};
        action.sa_handler = stacktrace_signal_handler;
        sigaction(SIGSEGV, &action, 0);
        sigaction(SIGABRT, &action, 0);
        sigaction(SIGINT, &action, 0);
    } // set_unrecoverable_signal_handlers
} // namespace irods

