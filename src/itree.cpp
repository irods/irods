#include <irods/rodsClient.h>
#include <irods/client_connection.hpp>
#include <irods/irods_client_api_table.hpp>
#include <irods/irods_pack_table.hpp>
#include <irods/irods_parse_command_line_options.hpp>
#include <irods/filesystem.hpp>
#include <irods/rcMisc.h>
#include <irods/rodsPath.h>

#include <fmt/core.h>
#include <fmt/color.h>

#include <boost/program_options.hpp>
#include <nlohmann/json.hpp>

#include <regex>
#include <stdexcept>
#include <string>
#include <variant>

#include <fnmatch.h>

// clang-format off
namespace po   = boost::program_options;
namespace fs   = irods::experimental::filesystem;
namespace ix   = irods::experimental;
namespace json = nlohmann;

using pattern_matcher = std::variant<int,               // no option specified
                                     std::string,       // glob-style pattern specified
                                     std::regex>;       // regex-style pattern specified

auto print_dir(const fs::path&,
               const po::variables_map&,
               irods::experimental::client_connection&) -> void;
// clang-format on

auto correct_path(const po::variables_map&, rodsEnv&) -> fs::path;
auto print_usage() -> void;
auto get_json(const fs::path&, const po::variables_map&, ix::client_connection&, unsigned int depth) -> json::json;
auto contains_pattern(const pattern_matcher& pm) -> bool;

// ANSI escape numbers. For setting the colors.
static const auto directory_color=fmt::color::blue;
static const auto file_color = fmt::color::green;
static rodsEnv env;
static pattern_matcher matcher;
static pattern_matcher exclude_matcher;
static unsigned int collections = 0, objects = 0;
static std::uintmax_t total_size = 0;


int main(int argc, char** argv){
    set_ips_display_name("itree");
    po::options_description desc{""};
    po::positional_options_description pod;
    pod.add("dir",1);
    desc.add_options()
        ("help,h", "Display help")
        ("dir", po::value<std::string>()->default_value("."), "The initial collection to render (defaults to the current working collection)")
        ("collections-only,c", po::bool_switch(), "Only list collections")
        ("fullpath,f", po::bool_switch(), "print the full path of each item listed")
        ("depth,L", po::value<int>()->default_value(1000), "Limit the depth of the listing.")
        //This does not function because the ownership variable is initialized to the
        //original uploader of the data_object
        //        ("owner,o", po::bool_switch(), "Display the owner along with the collection")
        ("pattern,P", po::value<std::string>(), "Filter files by a filename glob")
        ("pattern-regex,p", po::value<std::string>(), "Filter files by a regexp")
        ("size,s", po::bool_switch(),"Display the size of each data object")
        ("ignore,I", po::value<std::string>(), "Ignore matching data objects (using glob pattern)")
        ("ignore-regex,i", po::value<std::string>(), "Ignore matching data objects (using regexp pattern).")
        ("regex-extended-syntax,x", po::bool_switch(), "Regular expressions use extended syntax.")
        ("classify,F", po::bool_switch(), "Display a / at the end of listings of collections")
        ("indent", po::value<unsigned int>()->default_value(2), "The number of spaces each level of nested collection adds.")
        // Currently this does not work because the object_status is not initialized with the necessary information to
        // display this properly.
        //        ("acl,A", po::bool_switch(), "Print the access control information for each object")
        ("json,J", po::bool_switch(), "Produce json instead of a human readable output")
        ("color,C", po::bool_switch(), "Print in color (requires ansi terminal)");
    try{
        po::variables_map vm;
        po::store(po::command_line_parser(argc,argv).options(desc).positional(pod).run(),vm);
        po::notify(vm);
        if( vm.count("help") ) {
            print_usage();
            return 0;
        }

        auto regex_syntax_choice = std::regex::basic;
        if (vm["regex-extended-syntax"].as<bool>()) {
            regex_syntax_choice = std::regex::extended;
        }
        if (vm.count("pattern-regex")) {
            matcher = std::regex(vm["pattern-regex"].as<std::string>(),
                                 regex_syntax_choice | std::regex::optimize );
        }
        if (vm.count("pattern")) {
            if (contains_pattern(matcher)) {
                std::cerr << "Incompatible options: --pattern and --pattern-regex cannot be used together\n";
                exit(2);
            }
            matcher = vm["pattern"].as<std::string>();
        }
        if (vm.count("ignore-regex")) {
            exclude_matcher = std::regex(vm["ignore-regex"].as<std::string>(),
                                         regex_syntax_choice | std::regex::optimize);
        }
        if (vm.count("ignore")) {
            if (contains_pattern(exclude_matcher)) {
                std::cerr << "Incompatible options: --ignore and --ignore-regex cannot be used together\n";
                exit(2);
            }
            exclude_matcher = vm["ignore"].as<std::string>();
        }

        if (getRodsEnv(&env) < 0) {
            std::cerr << "Error: Could not get iRODS environment.\n";
            return 1;
        }

        auto path = correct_path(vm, env);
        load_client_api_plugins();
        irods::experimental::client_connection conn;

        if (!fs::client::is_collection(conn, path)) {
            std::cerr << "Error: The specified path does not refer to a collection.\n";
            return 1;
        }

        if (vm["json"].as<bool>()) {
            auto value = get_json(path, vm, conn, vm["depth"].as<int>() );
            json::json top;
            top.push_back(value);
            json::json report = {{"type","report"}, {"collections",collections},{"data_objects",objects}};

            if (vm["size"].as<bool>()) {
                report["size"] = total_size;
            }

            top.push_back(report);
            std::cout << top.dump(2) << "\n";
        }
        else {
            print_dir(path, vm, conn);
        }
    }
    catch (const irods::exception& e) {
        std::cerr << "Error: " << e.client_display_what() << "\n";
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Error:" << e.what() << "\n";
        return 1;
    }

    return 0;

}

const char* permission_type_string(fs::perms p) {
    using perms=fs::perms;
    switch(p){
    case perms::null:
        return "null";
    case perms::read_metadata:
        return "read_metadata";
    case perms::read_object:
    case perms::read:
        return "read";
    case perms::create_metadata:
        return "create_metadata";
    case perms::modify_metadata:
        return "modify_metadata";
    case perms::delete_metadata:
        return "delete_metadata";
    case perms::create_object:
        return "create_object";
    case perms::modify_object:
    case perms::write:
        return "write";
    case perms::delete_object:
        return "delete_object";
    case perms::own:
        return "own";
    default:
        return "unsupported";
    }
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>; // deduction guide, supposedly not needed in C++20
                                                              // (but still needed in clang 13 using -std=gnu++20).

// Helper function to invoke the right match type (regex / glob) based on std::variant contents.

auto flexible_match(const std::string& obj_name, const pattern_matcher& pattern_variant) -> bool
{
    auto matches = overloaded{
        [](const auto &)->bool { return false; },
        [&obj_name](const std::string& glob)->bool { return !fnmatch(glob.c_str(), obj_name.c_str(), 0); },
        [&obj_name](const std::regex& rgx)->bool { return std::regex_match(obj_name, rgx); }
    };
    return std::visit(matches, pattern_variant);
}

// Convenience function to wrap the logic for matching/ignoring.
auto object_matches(const fs::client::collection_entry& entry) -> bool {
    const auto obj_name = entry.path().object_name().string();

    // consider --ignore* and --pattern* as logically AND'ed together
    return ((!contains_pattern(exclude_matcher) || !flexible_match(obj_name, exclude_matcher)) &&
            (!contains_pattern(matcher) || flexible_match(obj_name, matcher)));
}

// Collection iterators don't populate the permissions field on the object status at this time.
// If that should change this may still be useful.
auto print_acl(const fs::client::collection_entry& e, const po::variables_map& vm){
    std::vector<std::string> elements;
    size_t size = 0;
    auto status = e.status();
    auto permissions = status.permissions();
    for(const auto& perm: permissions){
        elements.emplace_back(fmt::format("{}#{}:{} - {}",perm.name, perm.zone,
                                          permission_type_string(perm.prms),
                                          perm.type));
        size += elements.back().size();
    }
    std::string val;
    val.reserve(size+elements.size());
    for(const auto& element : elements){
        val += element+",";
    }
    std::cout <<" - ACL: " << val;
}

auto directory_depth(const fs::path& origin, const fs::path& descendant) -> int{
    int depth = 0;
    auto d = descendant;
    while( origin != d ) {
        d = d.parent_path();
        depth++;
    }
    return depth;
}

auto print_entry(const fs::client::collection_entry& e, const po::variables_map& vm, int depth){
    fmt::print("{:{}}","",depth * vm["indent"].as<unsigned int>());
    auto disp = e.path();
    if( !vm["fullpath"].as<bool>() ){
        disp=e.path().object_name();
    }
    std::string suffix;
    if( vm["classify"].as<bool>() && e.is_collection() ) {
        suffix = "/";
    }
    if( vm["color"].as<bool>() ) {
        fmt::print(fmt::fg(e.is_collection()?directory_color:file_color),
                   "{}{}",
                   static_cast<std::string>(disp),suffix);
    } else {
        fmt::print("{}{}",
                   static_cast<std::string>(disp),
                   suffix);
    }
    if ( vm.count("owner") && !e.owner().empty() ) {
        if( !vm["color"].as<bool>() ) {
            fmt::print(": Owned by {}",e.owner());
        } else {
            fmt::print(fmt::fg(fmt::color::white),": Owned By ");
            fmt::print(
                       fmt::fg(e.owner()==env.rodsUserName?fmt::color::green:fmt::color::red),
                       e.owner());
        }
    }
    if( e.is_data_object() && vm["size"].as<bool>() ) {
        fmt::print(" {} bytes", e.data_size());
    }
    if(vm.count("acl")){
        //        print_acl(e,vm);
    }
    std::cout << "\n";
}

auto print_dir(const fs::path& path,
               const po::variables_map& vm,
               irods::experimental::client_connection& conn) -> void {
    auto fsiter=fs::client::recursive_collection_iterator{conn,path,fs::client::collection_options::skip_permission_denied};
    const int max_depth = vm["depth"].as<int>();
    const bool fullpath = vm["fullpath"].as<bool>();
    const bool classify = vm["classify"].as<bool>();
    const char* suffix = (classify || path.object_name().string().empty())?"/":"";

    std::uintmax_t total_size = 0;
    if( vm["color"].as<bool>() ) {
        fmt::print(fmt::fg(directory_color),"{}{}\n",
                   fullpath?path.c_str():path.object_name().c_str(), suffix);
    }else{
        fmt::print("{}{}\n",
                   fullpath?path.c_str(): path.object_name().c_str(), suffix);
    }
    //    unsigned int objects = 0, collections = 0;
    for(auto &&iter:fsiter){
        auto current_depth = directory_depth(path,iter.path());
        auto is_dir = iter.is_collection();
        if( !is_dir ){
            if( vm["collections-only"].as<bool>() || !object_matches( iter ) ) {
                continue;
            }
        }
        if( current_depth  > max_depth ){
            continue;
        }
        (is_dir ? collections : objects)++;
        total_size += iter.data_size();
        print_entry(iter, vm, current_depth);
    }
    if( vm["size"].as<bool>() ){
        fmt::print("Found {} collections and {} data objects, taking up {} bytes total\n",
                   collections, objects,
                   total_size);
    } else {
        fmt::print("Found {} collections and {} data objects\n",
                   collections, objects);
    }
}

auto correct_path(const po::variables_map& pm, rodsEnv& env) -> fs::path {
    const auto path = pm["dir"].as<std::string>();

    RodsPath input{};
    rstrcpy(input.inPath, path.data(), MAX_NAME_LEN);

    if (parseRodsPath(&input, &env) != 0) {
        throw std::invalid_argument{fmt::format("Invalid path [{}].", path)};
    }

    auto* escaped_path = escape_path(input.outPath);
    fs::path p = escaped_path;
    std::free(escaped_path);

    return p;
}

auto print_usage() -> void {
    fmt::print(
R"_(itree - List contents of collections in a tree-like format.

Usage: itree [OPTION]... COLLECTION

When using this command, avoid listing very large collections. Failing to
follow this advice can result in unexpected failures. Consider listing smaller
collections or using --depth to limit the number of collections traversed.

Options:
  -c, --collections-only
                  Only list collections.
  -C, --color     Print in color (requires ANSI terminal).
  -f, --fullpath  Print the full path of each item.
  -F, --classify  Display a forward slash (/) at the end of collections.
  -h, --help      Display this help message and exit.
  -i, --ignore-regex=PATTERN
                  Ignore data objects matching a regular expression.
  -I, --ignore=PATTERN
                  Ignore data objects matching a file glob pattern.
      --indent=INTEGER
                  The number of spaces used for indenting nested collections.
                  (defaults to 2).
  -J, --json      Print collection tree as JSON.
  -L, --depth=INTEGER
                  Limit the depth of the listing (defaults to 1000).
  -p, --pattern-regex=PATTERN
                  Filter data objects by a regular expression.
  -P, --pattern=PATTERN
                  Filter data objects by a file glob pattern.
  -s, --size      Display the size of each data object.
  -x, --regex-extended-syntax
                  Enable extended syntax for regular expressions.
)_");

    printReleaseInfo("itree");
}

auto contents_size(const json::json& value) -> std::uintmax_t {
    std::uintmax_t total = 0;
    for(const auto& child : value["contents"]) {
        total += child["size"].get<std::uintmax_t>();
    }
    return total;
}

auto get_json(const fs::path& path, const po::variables_map& vm, ix::client_connection& conn, unsigned int depth ) -> json::json  {
    auto fsiter = fs::client::collection_iterator(conn, path);
    json::json value;
    value["type"] = "collection";
    if( vm["fullpath"].as<bool>() || path.object_name().string().empty() ) {
        value["name"] = path;
    } else {
        value["name"] = path.object_name();
    }
    value["contents"] = json::json::value_t::array;
    if( depth == 0 ) {
        return value;
    }
    for(const auto& object: fsiter) {
        if( object.is_collection() ) {
            json::json sub_coll;
            sub_coll = get_json(object.path(), vm, conn, depth - 1);
            value["contents"].emplace_back(std::move(sub_coll));
            collections++;
        } else {
            if(  vm["collections-only"].as<bool>() || !object_matches(object) ) {
                continue;
            }
            json::json data_object;
            if( vm["fullpath"].as<bool>() ){
                data_object["name"] = object.path();
            } else {
                data_object["name"] = object.path().object_name().string();
            }
            data_object["type"] = "data_object";

            if( vm["size"].as<bool>() ) {
                data_object["size"] = object.data_size();
            }
            if( vm.count("acl") ) {
                json::json permissions;
                for ( auto& perm : object.status().permissions() ) {
                    json::json permission;
                    permission["bearer_name"] = perm.name;
                    permission["zone"] = perm.zone;
                    permission["access_level"] = permission_type_string(perm.prms);
                    permission["type"] = perm.type;
                    permissions.emplace_back(permission);
                }
                data_object["acl"]=permissions;
            }
            value["contents"].emplace_back( std::move(data_object) );
            total_size += object.data_size();
            objects++;
        }
    }
    if( vm["size"].as<bool>() ) {
        value["size"] = contents_size(value);
    }
    return value;
}

auto contains_pattern(const pattern_matcher& pm) -> bool {
    return !std::holds_alternative<int>(pm);
}
