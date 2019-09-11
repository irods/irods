#include "rodsClient.h"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "connection_pool.hpp"
#include "dstream.hpp"
#include "transport/default_transport.hpp"
#include "filesystem.hpp"

#include <iostream>
#include <string>
#include <chrono>

namespace io = irods::experimental::io;
namespace fs = irods::experimental::filesystem;

int cleanup(irods::connection_pool::connection_proxy& _conn, const fs::path& _path);

int main()
{
    auto& api_table = irods::get_client_api_table();
    auto& pack_table = irods::get_pack_table();
    init_api_table(api_table, pack_table);

    rodsEnv env;
    if (getRodsEnv(&env) != 0) {
        std::cerr << "Error: cannot get iRODS environment\n";
        return 1;
    }

    using namespace std::string_literals;

    const std::string expected_text = "This is a test data object written by the iRODS post install binary.";
    const auto unique_id = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    const auto data_obj_path = fs::path{env.rodsHome} / ("post_install_test." + unique_id + ".txt");

    irods::connection_pool conn_pool{1, env.rodsHost, env.rodsPort, env.rodsUserName, env.rodsZone, 600};
    auto conn = conn_pool.get_connection();

    // Create a new data object and write some bytes to it.
    {
        io::client::default_transport xport{conn};
        io::odstream out{xport, data_obj_path};

        if (!out) {
            std::cerr << "Error: cannot open data object for writing [" << data_obj_path << "]\n";
            return 1;
        }

        out << expected_text;
    }

    // Check that the new data object contains the correct content.
    {
        io::client::default_transport xport{conn};
        io::idstream in{xport, data_obj_path};

        if (!in) {
            std::cerr << "Error: cannot open data object for reading [" << data_obj_path << "]\n";
            return 1;
        }

        std::string line;
        while (std::getline(in, line));

        if (expected_text != line) {
            std::cerr << "Error: data read does not match what was written\n";
            cleanup(conn, data_obj_path);
            return 1;
        }
    }

    return cleanup(conn, data_obj_path);
}

int cleanup(irods::connection_pool::connection_proxy& _conn, const fs::path& _path)
{
    try {
        if (!fs::client::remove(_conn, _path, fs::remove_options::no_trash)) {
            std::cerr << "Error: cannot remove data object [" << _path << "]\n";
            return 1;
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
