#include "irods/private/genquery1_driver.hpp"

#include <sstream>

namespace irods::experimental::genquery1
{
    auto driver::parse(const std::string&_s) -> int
    {
        location.initialize();

        std::istringstream iss{_s};
        lexer.switch_streams(&iss);

        gq1::parser p{*this};
        return p.parse();
    } // driver::parse
} // namespace irods::experimental::genquery1
