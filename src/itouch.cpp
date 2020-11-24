#include <irods/rodsClient.h>
#include <irods/client_connection.hpp>
#include <irods/irods_version.h>
#include <irods/touch.h>
#include <irods/irods_exception.hpp>

#include "utility.hpp"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <fmt/format.h>
#include <json.hpp>

#include <string>
#include <string_view>
#include <exception>

namespace po = boost::program_options;

auto print_usage_info() noexcept -> void;

auto print_version_info() noexcept -> void;

auto to_json_string(const rodsEnv& _env, const po::variables_map& _vm) -> std::string;

auto canonical(const std::string_view _path, const rodsEnv& _env) -> std::optional<std::string>;

int main(int _argc, char* _argv[])
{
    utils::set_ips_display_name(boost::filesystem::path{_argv[0]}.filename().c_str());

    rodsEnv env{};

    if (getRodsEnv(&env) < 0) {
        fmt::print(stderr, "Error: Could not get iRODS environment.\n");
        return 1;
    }

    po::options_description desc{""};
    desc.add_options()
        ("no-create,c", "")
        ("replica,n", po::value<int>(), "")
        ("resource,R", po::value<std::string>(), "")
        ("reference,r", po::value<std::string>(), "")
        ("seconds-since-epoch,s", po::value<int>(), "")
        ("logical-path", po::value<std::string>(), "")
        ("help,h", "")
        ("version,v", "");

    po::positional_options_description pod;
    pod.add("logical-path", 1);

    try {
        po::variables_map vm;
        po::store(po::command_line_parser(_argc, _argv).options(desc).positional(pod).run(), vm);
        po::notify(vm);

        if (vm.count("help")) {
           print_usage_info(); 
           return 0;
        }

        if (vm.count("version")) {
            print_version_info();
            return 0;
        }

        load_client_api_plugins();

        const auto json_input = to_json_string(env, vm);

        irods::experimental::client_connection comm;
        RcComm& comm_ref = comm;

        if (const auto ec = rc_touch(&comm_ref, json_input.data()); ec < 0) {
            printErrorStack(comm_ref.rError);
        }

        return 0;
    }
    catch (const irods::exception& e) {
        fmt::print(stderr, "Error: {}\n", e.client_display_what());
        return 1;
    }
    catch (const std::exception& e) {
        fmt::print(stderr, "Error: {}\n", e.what());
        return 1;
    }
}

auto print_usage_info() noexcept -> void
{
    fmt::print(R"_(itouch - Change logical path timestamp

Usage: itouch [OPTION]... LOGICAL_PATH

Update the modification time of a logical path to the current time.

A LOGICAL_PATH argument that does not exist will be created as an empty
data object, unless -c is supplied.

If LOGICAL_PATH does not exist and no options are specified, the server
will create a data object at the resource defined by msiSetDefaultResc().

If multiple replicas exist and a target replica is not specified via a
replica number or leaf resource, the latest good replica will be updated.

Mandatory arguments to long options are mandatory for short options too.

Options:
  -c, --no-create  Do not create a data object.
  -n, --replica    The replica number of the replica to update.  This
                   option applies to data objects only. Cannot be used
                   with -R.
  -R, --resource   The leaf resource that contians the replica to update.
                   This option applies to data objects only. If the data
                   object does not exist and this option is used, the
                   replica will be created on the resource specified.
                   Cannot be used with -n.
  -r, --reference=LOGICAL_PATH
                   Use the modification time of LOGICAL_PATH instead
                   of the current time. Cannot be used with -s.
  -s, --seconds-since-epoch=SECONDS
                   Use SECONDS instead of the current time. Cannot be
                   used with -r.
  -h, --help       Display this help message and exit.
  -v, --version    Display version information and exit.
)_");

    printReleaseInfo("itouch");
}

auto print_version_info() noexcept -> void
{
    // The empty string argument following the version macros exists so that the
    // fourth format placeholder is satisfied.
    fmt::print("iRODS Version {}.{}.{}{: >16}itouch\n", IRODS_VERSION_MAJOR, IRODS_VERSION_MINOR, IRODS_VERSION_PATCHLEVEL, "");
}

auto to_json_string(const rodsEnv& _env, const po::variables_map& _vm) -> std::string
{
    using json = nlohmann::json;

    json j;

    if (_vm.count("logical-path")) {
        std::string path;

        if (auto v = canonical(_vm["logical-path"].as<std::string>(), _env); v) {
            path = *v;
        }
        else {
            THROW(SYS_INVALID_INPUT_PARAM, "Failed to convert path to absolute path.");
        }

        j["logical_path"] = path;
    }

    j["options"]["no_create"] = (_vm.count("no-create") > 0);

    if (_vm.count("replica")) {
        j["options"]["replica_number"] = _vm["replica"].as<int>();
    }

    if (_vm.count("resource")) {
        j["options"]["leaf_resource_name"] = _vm["resource"].as<std::string>();
    }

    if (_vm.count("seconds-since-epoch")) {
        j["options"]["seconds_since_epoch"] = _vm["seconds-since-epoch"].as<int>();
    }

    if (_vm.count("reference")) {
        std::string path;

        if (auto v = canonical(_vm["reference"].as<std::string>(), _env); v) {
            path = *v;
        }
        else {
            THROW(SYS_INVALID_INPUT_PARAM, "Failed to convert path to absolute path.");
        }

        j["options"]["reference"] = path;
    }

    return j.dump();
}

auto canonical(const std::string_view _path, const rodsEnv& _env) -> std::optional<std::string>
{
    rodsPath_t input{};
    rstrcpy(input.inPath, _path.data(), MAX_NAME_LEN);

    if (parseRodsPath(&input, const_cast<rodsEnv*>(&_env)) != 0) {
        return std::nullopt;
    }

    auto* escaped_path = escape_path(input.outPath);
    std::optional<std::string> p = escaped_path;
    std::free(escaped_path);

    return p;
}

