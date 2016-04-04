#ifndef __IRODS_ASSERT_HPP__
#define __IRODS_ASSERT_HPP__

namespace irods {
// =-=-=-=-=-=-=-
/// @brief evaluate the expr, call stacktrace and potentially exit if necessary
    bool assert( bool _expr,           // expression to assert
                 bool _exit = false ); // call exit( -1 )

}; // namespace irods


#endif // __IRODS_ASSERT_HPP__



