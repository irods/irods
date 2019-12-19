#include "rodsClient.h"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "connection_pool.hpp"
#include "dstream.hpp"
#include "transport/default_transport.hpp"
#include "rcMisc.h"
#include "rodsPath.h"

#include "boost/program_options.hpp"

#include <iostream>
#include <string>
#include <array>
#include <tuple>
#include <optional>

namespace io = irods::experimental::io;
namespace po = boost::program_options;

using int_type = std::int64_t;

// clang-format off
constexpr auto buffer_size = 4 * 1024 * 1024;
constexpr auto all_bytes   = -1;
// clang-format on

auto usage() -> void;
auto check_input_arguments(const po::variables_map& vm) -> std::tuple<bool, int>;
auto canonical(const std::string& _path, rodsEnv& _env) -> std::optional<std::string>;
auto stream_bytes(std::istream& in, std::ostream& out, int_type count) -> void;
auto read_data_object(rodsEnv& env, const po::variables_map& vm, io::client::default_transport& tp) -> int;
auto write_data_object(rodsEnv& env, const po::variables_map& vm, io::client::default_transport& tp) -> int;

int main(int argc, char* argv[])
{
    if (argc == 1) {
        std::cerr << "Error: Invalid number of arguments.\n";
        return 1;
    }

    po::options_description desc{"Options are"};
    desc.add_options()
        ("help,h", "Produce this message.")
        ("offset,o", po::value<int_type>()->default_value(0), "The position of the read/write pointer.")
        ("count,c", po::value<int_type>()->default_value(all_bytes), "The number of bytes to read/write.")
        ("no-trunc", po::bool_switch(), "Do not truncate the data object on write.")
        ("append,a", po::bool_switch(), "Append to the data object on write.")
        ("stream_operation", po::value<std::string>(), "The stream operation (e.g. read or write).")
        ("logical_path", po::value<std::string>(), "The logical path of a data object.");

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

        auto& pack_table = irods::get_pack_table();
        auto& api_table = irods::get_client_api_table();
        init_api_table(api_table, pack_table);

        rodsEnv env;
        if (getRodsEnv(&env) < 0) {
            std::cerr << "Error: Could not get iRODS environment.\n";
            return 1;
        }

        irods::connection_pool conn_pool{1, env.rodsHost, env.rodsPort, env.rodsUserName, env.rodsZone, 600};
        auto conn = conn_pool.get_connection();
        io::client::default_transport tp{conn};

        const auto stream_operation = vm["stream_operation"].as<std::string>();

        if ("read" == stream_operation) {
            return read_data_object(env, vm, tp);
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
    std::cout << "Usage: istream read [-o INTEGER] [-c INTEGER] LOGICAL_PATH\n"
                 "Usage: istream write [-o INTEGER] [-c INTEGER] [--no-trunc] [-a] LOGICAL_PATH\n"
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
                 "Options are:\n"
                 "-o, --offset    The number of bytes to skip within the data object before\n"
                 "                reading/writing.  Defaults to zero.\n"
                 "-c, --count     The number of bytes to read/write.  Defaults to all bytes.\n"
                 "-a, --append    Appends bytes read from stdin to the data object.\n"
                 "    --no-trunc  Does not truncate the data object.  Disables creation of data\n"
                 "                objects.\n"
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

auto stream_bytes(std::istream& in, std::ostream& out, int_type count) -> void
{
    std::array<char, buffer_size> buf;

    if (all_bytes == count) {
        while (in && out) {
            in.read(buf.data(), buf.size());
            out.write(buf.data(), in.gcount());
        }

        if (!in.eof()) {
            throw std::ios::failure{"Failed to read all bytes"};
        }

        if (!out) {
            throw std::ios::failure{"Failed to write all bytes to data object"};
        }

        return;
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
        throw std::ios::failure{"Failed to read requested number of bytes"};
    }

    if (!out) {
        throw std::ios::failure{"Failed to write requested number of bytes to data object"};
    }
}

auto read_data_object(rodsEnv& env, const po::variables_map& vm, io::client::default_transport& tp) -> int
{
    if (vm["append"].as<bool>()) {
        std::cerr << "Error: Invalid argument on read [append].\n";
        return 1;
    }

    if (vm["no-trunc"].as<bool>()) {
        std::cerr << "Error: Invalid argument on read [no-trunc].\n";
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

    io::idstream in{tp, path};

    if (!in) {
        std::cerr << "Error: Cannot open data object.\n";
        return 1;
    }

    if (!in.seekg(vm["offset"].as<int_type>())) {
        std::cerr << "Error: Could not seek to offset.\n";
        return 1;
    }

    stream_bytes(in, std::cout, vm["count"].as<int_type>());

    return 0;
}

auto write_data_object(rodsEnv& env, const po::variables_map& vm, io::client::default_transport& tp) -> int
{
    auto mode = std::ios_base::out;

    if (vm["append"].as<bool>()) {
        mode = std::ios_base::app;
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

    io::odstream out{tp, path, mode};

    if (!out) {
        std::cerr << "Error: Cannot open data object.\n";
        return 1;
    }

    if (!out.seekp(vm["offset"].as<int_type>())) {
        std::cerr << "Error: Could not seek to offset.\n";
        return 1;
    }

    stream_bytes(std::cin, out, vm["count"].as<int_type>());

    return 0;
}

