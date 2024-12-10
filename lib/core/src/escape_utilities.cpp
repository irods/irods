#include "irods/escape_utilities.hpp"

#include <boost/algorithm/string/replace.hpp>

namespace irods
{
    auto single_quotes_to_hex(const std::string& _s) -> std::string
    {
        return boost::replace_all_copy(_s, "'", "\\x27");
    } // single_quotes_to_hex
} // namespace irods
