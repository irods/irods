#ifndef IRODS_STACKTRACE_HPP
#define IRODS_STACKTRACE_HPP

#include <string>

namespace irods
{
    class stacktrace final
    {
    public:
        std::string dump() const;
    }; // class stacktrace
} // namespace irods

#endif // IRODS_STACKTRACE_HPP
