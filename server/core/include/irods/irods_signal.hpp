#ifndef IRODS_SIGNAL_HPP
#define IRODS_SIGNAL_HPP

#include <string>

namespace irods
{
    /// A special tag that instructs the stacktrace file processor to not
    /// touch the file.
    ///
    /// \since 4.3.0
    extern const char* const STACKTRACE_NOT_READY_FOR_LOGGING_SUFFIX;

    /// std::string containing the path of the directory where stacktraces are
    /// stored.
    ///
    /// \since 4.3.0
    extern const std::string STACKTRACE_DIR;

    /// Sets signal handlers for SIGSEGV, SIGABRT, and SIGINT.
    ///
    /// \since 4.3.0
    void set_unrecoverable_signal_handlers();
} // namespace irods

#endif // IRODS_SIGNAL_HPP
