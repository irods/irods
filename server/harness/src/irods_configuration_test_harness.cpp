#include <iostream>

#include "irods/irods_server_properties.hpp"

int main(int argc, char* argv[])
{
    const auto config_handle{irods::server_properties::instance().map()};
    std::cout << config_handle.get_json().dump(4).c_str() << '\n';
    return 0;
} // main
