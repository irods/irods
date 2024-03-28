#include "irods/genquery2_driver.hpp"

#include <sstream>

namespace irods::experimental::genquery2
{
    auto driver::parse(const std::string& _s) -> int
    {
        location.initialize();

        std::istringstream iss{_s};
        lexer.switch_streams(&iss);

        yy::parser p{*this};
        return p.parse();
    } // driver::parse
} // namespace irods::experimental::genquery2
