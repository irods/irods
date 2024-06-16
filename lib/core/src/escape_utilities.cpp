#include "irods/escape_utilities.hpp"

#include "irods/irods_exception.hpp"
#include "irods/rodsErrorTable.h"

#include <boost/algorithm/string/replace.hpp>
#include <fmt/format.h>

#include <exception>

namespace irods
{
    auto single_quotes_to_hex(const std::string& _s) -> std::string
    {
        try {
            return boost::replace_all_copy(_s, "'", "\\x27");
        }
        catch (const std::exception& e) {
            THROW(SYS_LIBRARY_ERROR, fmt::format("{}: {}", __func__, e.what()));
        }
    } // single_quotes_to_hex
} // namespace irods
