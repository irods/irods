


#ifndef __EIRODS_ASSERT_H__
#define __EIRODS_ASSERT_H__

namespace eirods {
    // =-=-=-=-=-=-=-
    /// @brief evaluate the expr, call stacktrace and potentially exit if necessary
    bool assert( bool _expr,           // expression to assert
                 bool _exit = false ); // call exit( -1 ) 

}; // namespace eirods


#endif // __EIRODS_ASSERT_H__



