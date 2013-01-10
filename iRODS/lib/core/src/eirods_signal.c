/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "eirods_signal.h"
#include "eirods_stacktrace.h"

#include <signal.h>
#include <malloc.h>
#include <stdlib.h>

// Define signal handlers for e-irods

extern "C" {

    /// @brief Signal handler for seg faults
    static void segv_handler(
        int signal)
    {
        eirods::stacktrace st;
        st.trace();
        st.dump();
        exit(signal);
    }
    

    void register_handlers(void) {
        struct sigaction* action = (struct sigaction*)calloc(sizeof(struct sigaction), 0);
        action->sa_handler = segv_handler;
        sigaction(11, action, 0);
    }
};
