#include <iostream>

#include "irods_exception.hpp"
#include "irods_hostname.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Call with hostname argument" << std::endl;
        return -1;
    }

    try {
        const bool hostname_is_local = hostname_resolves_to_local_address(argv[1]);
        if (hostname_is_local) {
            std::cout << "yes" << std::endl;
            return 0;
        } else {
            std::cout << "no" << std::endl;
            return 1;
        }
    } catch ( const irods::exception& e ) {
        std::cerr << e.what() << std::endl;
    }
    return -1;
}
