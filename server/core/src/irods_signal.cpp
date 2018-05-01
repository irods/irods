#include "irods_signal.hpp"
#include "irods_stacktrace.hpp"

#include <string.h>
#include <signal.h>
#include <cstdlib>

#include <iostream>

// Define signal handlers for irods

extern "C" {

    /// @brief Signal handler for seg faults
    static void segv_handler(
        int signal ) {
        std::cerr << "Caught signal [" << signal << "]. Dumping stacktrace and exiting" << std::endl;
        std::cerr << irods::stacktrace().dump();
        exit(signal);
    }

    void register_handlers( void ) {
        struct sigaction action;
        memset(&action, 0, sizeof(action));
        action.sa_handler = segv_handler;
        sigaction( SIGSEGV, &action, nullptr );
        sigaction( SIGABRT, &action, nullptr );
        sigaction( SIGINT, &action, nullptr );
    }
}
