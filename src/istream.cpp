#include <irods/rodsClient.h>
#include <irods/client_connection.hpp>
#include <irods/dstream.hpp>
#include <irods/transport/default_transport.hpp>
#include <irods/rcMisc.h>
#include <irods/rodsPath.h>
#include <irods/irods_at_scope_exit.hpp>
#include <irods/replica.hpp>

#include "utility.hpp"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <array>
#include <tuple>
#include <optional>
#include <algorithm>

namespace io = irods::experimental::io;
namespace ir = irods::experimental::replica;
namespace po = boost::program_options;

using int_type = std::int64_t;

// clang-format off
constexpr auto buffer_size = 4 * 1024 * 1024;
constexpr auto all_bytes   = std::numeric_limits<int_type>::max();
// clang-format on

auto usage() -> void;

auto check_input_arguments(const po::variables_map& vm) -> std::tuple<bool, int>;

auto canonical(const std::string& path, rodsEnv& env) -> std::optional<std::string>;

auto stream_bytes(std::istream& in, std::ostream& out, int_type count) -> int;

auto read_data_object(rodsEnv& env,
                      const po::variables_map& vm,
                      irods::experimental::client_connection& conn,
                      io::client::default_transport& tp) -> int;

auto write_data_object(rodsEnv& env, const po::variables_map& vm, io::client::default_transport& tp) -> int;

int main(int argc, char* argv[])
{
    utils::set_ips_display_name(boost::filesystem::path{argv[0]}.filename().c_str());

    po::options_description desc{""};
    desc.add_options()
        ("help,h", "")
        ("resource,R", po::value<std::string>(), "")
        ("replica,n", po::value<int>(), "")
        ("offset,o", po::value<int_type>()->default_value(0), "")
        ("count,c", po::value<int_type>()->default_value(all_bytes), "")
        ("no-trunc", po::bool_switch(), "")
        ("append,a", po::bool_switch(), "")
        ("checksum,k", po::bool_switch(), "")
        ("stream_operation", po::value<std::string>(), "")
        ("logical_path", po::value<std::string>(), "");

    po::positional_options_description pod;
    pod.add("stream_operation", 1);
    pod.add("logical_path", 1);

    try {
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).positional(pod).run(), vm);
        po::notify(vm);

        if (const auto [return_ec, ec] = check_input_arguments(vm); return_ec) {
            return ec;
        }

        load_client_api_plugins();

        rodsEnv env;
        if (getRodsEnv(&env) < 0) {
            std::cerr << "Error: Could not get iRODS environment.\n";
            return 1;
        }

        irods::experimental::client_connection conn;

        irods::at_scope_exit print_errors_on_exit{[&conn] {
            printErrorStack(static_cast<rcComm_t*>(conn)->rError);
        }};

        io::client::default_transport tp{conn};

        const auto stream_operation = vm["stream_operation"].as<std::string>();

        if ("read" == stream_operation) {
            return read_data_object(env, vm, conn, tp);
        }

        if ("write" == stream_operation) {
            return write_data_object(env, vm, tp);
        }

        std::cerr << "Error: Invalid stream operation [" << stream_operation << "]\n";

        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}

auto usage() -> void
{
    std::cout << "Usage: istream read [-R RESC_NAME] [-o INTEGER] [-c INTEGER] LOGICAL_PATH\n"
                 "Usage: istream read [-n REPLICA_NUMBER] [-o INTEGER] [-c INTEGER] LOGICAL_PATH\n"
                 "Usage: istream write [-R RESC_NAME] [-k] [-o INTEGER] [-c INTEGER] [--no-trunc] [-a] LOGICAL_PATH\n"
                 "Usage: istream write [-n REPLICA_NUMBER] [-k] [-o INTEGER] [-c INTEGER] [--no-trunc] [-a] LOGICAL_PATH\n"
                 "\n"
                 "Streams bytes to/from iRODS via stdin/stdout.\n"
                 "Reads bytes from the target data object and prints them to stdout.\n"
                 "Writes bytes from stdin to the target data object.\n"
                 "\n"
                 "Writing to a non-existent data object will create it by default.\n"
                 "Writing to an existing data object always truncates it.  Truncation can be\n"
                 "disabled by passing --no-trunc.  However, using --no-trunc disables creation\n"
                 "of data objects.\n"
                 "\n"
                 "If the client-side of the connection is interrupted before the data object\n"
                 "is closed, then the catalog will not be updated and the data object will have\n"
                 "a replica status of intermediate or stale.  Information such as data object\n"
                 "size and checksum will not be available.\n"
                 "\n"
                 "If the target data object does not reside on the connected server, istream\n"
                 "will not attempt to redirect to the server hosting the replica.  Traffic will\n"
                 "always flow through the connected server.\n"
                 "\n"
                 "Options:\n"
                 "-R, --resource  The root resource to read from or write to.  Cannot be used\n"
                 "                with --replica.\n"
                 "-n, --replica   The replica number of the replica to read from or write to.\n"
                 "                Replica numbers cannot be used to create new data objects.\n"
                 "                Cannot be used with --resource.\n"
                 "-o, --offset    The number of bytes to skip within the data object before\n"
                 "                reading/writing.  Value must be positive.  Defaults to zero.\n"
                 "-c, --count     The number of bytes to read/write.  Value must be positive.\n"
                 "                Defaults to all bytes.\n"
                 "-a, --append    Appends bytes read from stdin to the data object.  Implies\n"
                 "                --no-trunc.\n"
                 "    --no-trunc  Does not truncate the data object.  Disables creation of data\n"
                 "                objects.  Ignored by write operations when --append is set.\n"
                 "-k, --checksum  Compute checksum.\n"
                 "-h, --help      Prints this message\n";

    printReleaseInfo("istream");
}

auto check_input_arguments(const po::variables_map& vm) -> std::tuple<bool, int>
{
    if (vm.count("help")) {
        usage();
        return {true, 0};
    }

    const auto is_valid_operation = [](const auto& op)
    {
        const auto ops = {"read", "write"};
        return std::any_of(std::begin(ops), std::end(ops), [&op] (auto&& other_op) {
            return op == other_op;
        });
    };

    if (vm.count("stream_operation") == 0 || !is_valid_operation(vm["stream_operation"].as<std::string>())) {
        std::cerr << "Error: Missing stream operation.\n";
        return {true, 1};
    }

    if (vm.count("logical_path") == 0) {
        std::cerr << "Error: Missing logical path.\n";
        return {true, 1};
    }

    return {false, 0};
}

auto canonical(const std::string& path, rodsEnv& env) -> std::optional<std::string>
{
    rodsPath_t input{};
    rstrcpy(input.inPath, path.data(), MAX_NAME_LEN);

    if (parseRodsPath(&input, &env) != 0) {
        return std::nullopt;
    }

    auto* escaped_path = escape_path(input.outPath);
    std::optional<std::string> p = escaped_path;
    std::free(escaped_path);

    return p;
}

auto stream_bytes(std::istream& in, std::ostream& out, int_type count) -> int
{
    if (count < 0) {
        std::cerr << "Error: Invalid byte count.\n";
        return 1;
    }

    std::array<char, buffer_size> buf;

    if (all_bytes == count) {
        while (in && out) {
            in.read(buf.data(), buf.size());
            out.write(buf.data(), in.gcount());
        }

        if (!in.eof()) {
            std::cerr << "Error: Failed to read all bytes.\n";
            return 1;
        }

        if (!out) {
            std::cerr << "Error: Failed to write all bytes to data object.\n";
            return 1;
        }

        return 0;
    }

    int_type bytes_read = 0;

    for (int_type i = 0, n_reads = count / buffer_size; i < n_reads; ++i) {
        while (in && out) {
            in.read(buf.data(), buf.size());
            out.write(buf.data(), in.gcount());
            bytes_read += in.gcount();
        }
    }

    if (const auto remainder = count % buffer_size; remainder > 0 && in && out) {
        in.read(buf.data(), remainder);
        out.write(buf.data(), in.gcount());
        bytes_read += in.gcount();
    }

    if (bytes_read != count) {
        std::cerr << "Error: Failed to read requested number of bytes.\n";
        return 1;
    }

    if (!out) {
        std::cerr << "Error: Failed to write requested number of bytes to data object.\n";
        return 1;
    }

    return 0;
}

auto read_data_object(rodsEnv& env,
                      const po::variables_map& vm,
                      irods::experimental::client_connection& conn,
                      io::client::default_transport& tp) -> int
{
    if (vm["append"].as<bool>()) {
        std::cerr << "Error: Invalid option on read: --append\n";
        return 1;
    }

    if (vm["no-trunc"].as<bool>()) {
        std::cerr << "Error: Invalid option on read: --no-trunc.\n";
        return 1;
    }

    std::string path;

    if (auto v = canonical(vm["logical_path"].as<std::string>(), env); v) {
        path = *v;
    }
    else {
        std::cerr << "Error: Failed to convert path to absolute path.\n";
        return 1;
    }

    io::idstream in;

    if (vm.count("resource")) {
        if (vm.count("replica")) {
            std::cerr << "Error: --resource and --replica cannot be used together.\n";
            return 1;
        }

        in.open(tp, path, io::root_resource_name{vm["resource"].as<std::string>()});
    }
    else if (vm.count("replica")) {
        in.open(tp, path, io::replica_number{vm["replica"].as<int>()});
    }
    else {
        in.open(tp, path);
    }

    if (!in) {
        std::cerr << "Error: Cannot open data object.\n";
        return 1;
    }

    if (const auto offset = vm["offset"].as<int_type>(); offset >= 0) {
        if (!in.seekg(offset)) {
            std::cerr << "Error: Could not seek to offset.\n";
            return 1;
        }
    }
    else {
        std::cerr << "Error: Invalid byte offset.\n";
        return 1;
    }

    const auto count = std::min<int_type>(ir::replica_size<RcComm>(conn, path, in.replica_number().value),
                                          vm["count"].as<int_type>());

    if (const auto ec = stream_bytes(in, std::cout, count); ec) {
        return ec;
    }

    return 0;
}

auto write_data_object(rodsEnv& env, const po::variables_map& vm, io::client::default_transport& tp) -> int
{
    auto mode = std::ios_base::out;

    if (vm["append"].as<bool>()) {
        mode |= std::ios_base::app;
    }
    else if (vm["no-trunc"].as<bool>()) {
        mode |= std::ios_base::in;
    }

    std::string path;

    if (auto v = canonical(vm["logical_path"].as<std::string>(), env); v) {
        path = *v;
    }
    else {
        std::cerr << "Error: Failed to convert path to absolute path.\n";
        return 1;
    }

    io::odstream out;
    
    if (vm.count("resource")) {
        if (vm.count("replica")) {
            std::cerr << "Error: --resource and --replica cannot be used together.\n";
            return 1;
        }

        out.open(tp, path, io::root_resource_name{vm["resource"].as<std::string>()}, mode);
    }
    else if (vm.count("replica")) {
        out.open(tp, path, io::replica_number{vm["replica"].as<int>()}, mode);
    }
    else if (std::strlen(env.rodsDefResource) > 0) {
        out.open(tp, path, io::root_resource_name{env.rodsDefResource}, mode);
    }
    else {
        out.open(tp, path, mode);
    }

    if (!out) {
        std::cerr << "Error: Cannot open data object.\n";
        return 1;
    }

    if (const auto offset = vm["offset"].as<int_type>(); offset >= 0) {
        if (!out.seekp(offset)) {
            std::cerr << "Error: Could not seek to offset.\n";
            return 1;
        }
    }
    else {
        std::cerr << "Error: Invalid byte offset.\n";
        return 1;
    }

    if (const auto ec = stream_bytes(std::cin, out, vm["count"].as<int_type>()); ec) {
        return ec;
    }

    if (vm["checksum"].as<bool>() && out) {
        io::on_close_success input;
        input.compute_checksum = true;
        out.close(&input);
    }

    return 0;
}

