#include "irods/irods_stacktrace.hpp"

#include <boost/stacktrace.hpp>

namespace irods
{
    std::string stacktrace::dump() const
    {
        return to_string(boost::stacktrace::stacktrace{});
    }
} // namespace irods
