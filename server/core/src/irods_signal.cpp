#include "irods_signal.hpp"
#include "irods_stacktrace.hpp"
#include "irods_logger.hpp"

#include <string.h>
#include <signal.h>

#include <cstdlib>
#include <sstream>

// Define signal handlers for irods

extern "C"
{
    /// @brief Signal handler for seg faults
    static void segv_handler(int _signal)
    {
        using log = irods::experimental::log;

        std::stringstream ss;
        ss << "Caught signal [" << _signal << "]. Dumping stacktrace and exiting ...\n";

        log::server::error({{"log_message", ss.str()},
                            {"stacktrace", irods::stacktrace().dump()}});

        exit(_signal);
    }

    void register_handlers()
    {
        struct sigaction action;
        memset(&action, 0, sizeof(action));
        action.sa_handler = segv_handler;
        sigaction(SIGSEGV, &action, 0);
        sigaction(SIGABRT, &action, 0);
        sigaction(SIGINT, &action, 0);
    }
}
