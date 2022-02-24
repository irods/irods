#include <iostream>

#include "irods/irods_server_properties.hpp"

int main(int argc, char* argv[])
{
    std::cout << irods::server_properties::instance().map().dump(4).c_str() << std::endl;
    return 0;
} // main
