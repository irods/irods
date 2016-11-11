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
#if 0
        // RTS - output in signal handlers are unsafe - #3326 
        std::cerr << "Caught signal [" << signal << "]. Dumping stacktrace and exiting" << std::endl;
        std::cerr << irods::stacktrace().dump();
#endif
        exit( signal );
    }

    void register_handlers( void ) {
        struct sigaction action;
        memset( &action, 0, sizeof( action ) );
        action.sa_handler = segv_handler;
#ifndef RELEASE_FLAG
        sigaction( SIGSEGV, &action, 0 );
        sigaction( SIGABRT, &action, 0 );
        sigaction( SIGINT, &action, 0 );
#endif
    }
};
