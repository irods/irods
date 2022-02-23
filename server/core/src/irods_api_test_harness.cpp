#include "irods/rodsClient.h"
#include "irods/connection_pool.hpp"
#include "irods/experimental_plugin_framework.hpp"
#include "irods/irods_client_api_table.hpp"

int main(int argc, char* argv[])
{
    if(argc < 3) { return 1; }

    load_client_api_plugins();

    std::atomic_bool f{};
    std::cout << irods::experimental::api::client{}(
                     irods::make_connection_pool()->get_connection(), f,
                     [](const std::string&){}, json::parse(argv[1]), argv[2]);
} // main
