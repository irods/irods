#ifndef IRODS_SIGNAL_HPP
#define IRODS_SIGNAL_HPP

namespace irods
{
    /// A special tag that instructs the stacktrace file processor to not
    /// touch the file.
    ///
    /// \since 4.3.0
    extern const char* const STACKTRACE_NOT_READY_FOR_LOGGING_SUFFIX;

    /// Sets signal handlers for SIGSEGV, SIGABRT, and SIGINT.
    ///
    /// \since 4.3.0
    void set_unrecoverable_signal_handlers();
} // namespace irods

#endif // IRODS_SIGNAL_HPP
