


// =-=-=-=-=-=-=
// eirods includes
#include "eirods_assert.h"
#include "eirods_stacktrace.h"

// =-=-=-=-=-=-=
// needed for exit
#include "stdlib.h"

namespace eirods {

    // =-=-=-=-=-=-=
    // eirods includes
    bool assert( 
             bool _expr,
             bool _exit ) {
        // =-=-=-=-=-=-=-
        // marker to tell lcov to start skipping lines
        // LCOV_EXCL_START 
       
        // =-=-=-=-=-=-=-
        // only need to worry if _expr is false
        if( !_expr ) {
            // =-=-=-=-=-=-=-
            // bad expr, stacktrace
            stacktrace st;
            st.trace();
            st.dump();

            // =-=-=-=-=-=-=-
            // call exit as requested
            if( _exit ) {
                exit( -1 );
            } 

        } // if !_expr

        return _expr;

        // =-=-=-=-=-=-=-
        // marker to tell lcov to stop skipping lines
        // LCOV_EXCL_STOP 

    } // assert

}; // namespace eirods



