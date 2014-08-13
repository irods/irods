#include "irods_signal.hpp"
#include "irods_stacktrace.hpp"

#include <string.h>
#include <signal.h>
#include <stdlib.h>
#if defined(osx_platform)
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif

// Define signal handlers for irods

extern "C" {

    static struct sigaction* action = 0;

    /// @brief Signal handler for seg faults
    static void segv_handler(
        int signal ) {
        irods::stacktrace st;
        st.trace();
        st.dump();
        exit( signal );
    }


    void register_handlers( void ) {
        action = ( struct sigaction* )malloc( sizeof( struct sigaction ) );
        memset( action, 0, sizeof( struct sigaction ) );
        action->sa_handler = segv_handler;
#ifndef RELEASE_FLAG
        sigaction( 11, action, 0 );
        sigaction( 6, action, 0 );
        sigaction( 2, action, 0 );
#endif
    }

    void unregister_handlers() {
        free( action );
    }
};
