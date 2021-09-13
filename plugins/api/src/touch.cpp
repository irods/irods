#include "api_plugin_number.h"
#include "rodsDef.h"
#include "rcConnect.h"
#include "rodsPackInstruct.h"
#include "apiHandler.hpp"
#include "client_api_whitelist.hpp"

#include <functional>

#ifdef RODS_SERVER

//
// Server-side Implementation
//

#include "touch.h"

#include "rcMisc.h"
#include "rodsErrorTable.h"
#include "catalog_utilities.hpp"
#include "irods_exception.hpp"
#include "irods_server_api_call.hpp"
#include "irods_re_serialization.hpp"
#include "irods_resource_manager.hpp"
#include "rodsLog.h"

#define IRODS_IO_TRANSPORT_ENABLE_SERVER_SIDE_API
#include "transport/default_transport.hpp"
#include "dstream.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "replica.hpp"

#define IRODS_QUERY_ENABLE_SERVER_SIDE_API
#include "query_builder.hpp"

#include "fmt/format.h"
#include "json.hpp"

#include <string>
#include <string_view>
#include <chrono>

/*
 The expected JSON format:
 ~~~~~~~~~~~~~~~~~~~~~~~~~
 {
     // Must be an absolute path.
     // Cannot be empty.
     "logical_path": string, 

     // Optional set of options.
     // Allowed to be empty.
     // Not required to exist.
     "options": {
         // Defaults to false.
         "no_create": boolean,
         
         // Cannot be used with leaf_resource_name.
         // No default value.
         "replica_number": integer,
         
         // Cannot be used with replica_number.
         // Cannot be empty if present.
         // No default value.
         "leaf_resource_name": string,
         
         // Must be greater than or equal to zero.
         // Cannot be used with reference.
         // No default value.
         "seconds_since_epoch": integer,
         
         // Must be an absolute path.
         // Cannot be empty if present.
         // Cannot be used with seconds_since_epoch.
         // No default value.
         "reference": string
     }
 }
*/

extern irods::resource_manager resc_mgr;

namespace
{
    // clang-format off
    namespace ix = irods::experimental;
    namespace fs = irods::experimental::filesystem;

    using json                = nlohmann::json;
    using rep_type            = fs::object_time_type::rep;
    using replica_number_type = ix::replica::replica_number_type;

    // JSON Input Properties
    constexpr std::string_view prop_logical_path        = "logical_path";
    constexpr std::string_view prop_options             = "options";
    constexpr std::string_view prop_no_create           = "no_create";
    constexpr std::string_view prop_replica_number      = "replica_number";
    constexpr std::string_view prop_leaf_resource_name  = "leaf_resource_name";
    constexpr std::string_view prop_reference           = "reference";
    constexpr std::string_view prop_seconds_since_epoch = "seconds_since_epoch";
    // clang-format on

    //
    // Function Prototypes
    //

    template <typename ExpectedType>
    auto throw_if_incorrect_type(bool _check_value,
                                 const json& _object,
                                 const std::string_view _prop_name) -> void;

    auto throw_if_empty_string(bool _check_value,
                               const json& _object,
                               const std::string_view _prop_name) -> void;

    template <typename ExpectedType>
    auto throw_if_less_than_zero(bool _check_value,
                                 const json& _object,
                                 const std::string_view _prop_name) -> void;

    auto parse_json(const bytesBuf_t* _bbuf) -> json;

    auto throw_if_input_is_invalid(const json& _json_input) -> void;

    auto get_time(rsComm_t& _comm, const json& _json_input) -> fs::object_time_type;

    auto create_data_object_if_necessary(rsComm_t& _comm,
                                         const json& _json_input,
                                         const fs::path& _path) -> bool;

    auto get_replica_number(rsComm_t& _comm,
                            const json& _json_input,
                            const fs::path& _path) -> replica_number_type;

    auto rs_touch(rsComm_t* _comm, bytesBuf_t* _bbuf_input) -> int;

    auto call_touch(irods::api_entry* _api, rsComm_t* _comm, bytesBuf_t* _bbuf_input) -> int;

    //
    // Function Implementations
    //

    template <typename ExpectedType>
    auto throw_if_incorrect_type(bool _check_value,
                                 const json& _object,
                                 const std::string_view _prop_name) -> void
    {
        if (!_check_value) {
            return;
        }

        try {
            _object.at(_prop_name.data()).get<ExpectedType>();
        }
        catch (const json::exception&) {
            THROW(SYS_INVALID_INPUT_PARAM, fmt::format("Incorrect value for [{}].", _prop_name));
        }
    } // throw_if_incorrect_type

    auto throw_if_empty_string(bool _check_value,
                               const json& _object,
                               const std::string_view _prop_name) -> void
    {
        if (!_check_value) {
            return;
        }

        if (_object.at(_prop_name.data()).get_ref<const std::string&>().empty()) {
            THROW(INPUT_ARG_NOT_WELL_FORMED_ERR, fmt::format("[{}] cannot be empty.", _prop_name));
        }
    } // throw_if_empty_string

    template <typename ExpectedType>
    auto throw_if_less_than_zero(bool _check_value,
                                 const json& _object,
                                 const std::string_view _prop_name) -> void
    {
        if (!_check_value) {
            return;
        }

        if (_object.at(_prop_name.data()).get<ExpectedType>() < 0) {
            THROW(INPUT_ARG_NOT_WELL_FORMED_ERR, fmt::format("[{}] cannot be less than zero.", _prop_name));
        }
    } // throw_if_less_than_zero

    auto parse_json(const bytesBuf_t* _bbuf) -> json
    {
        if (!_bbuf) {
            THROW(SYS_NULL_INPUT, "Could not parse string (null pointer) into JSON.");
        }

        try {
            const std::string_view json_string(static_cast<const char*>(_bbuf->buf), _bbuf->len);
            return json::parse(json_string);
        }
        catch (const json::exception& e) {
            THROW(INPUT_ARG_NOT_WELL_FORMED_ERR, "Could not parse string into JSON.");
        }
    } // parse_json

    auto throw_if_input_is_invalid(const json& _json_input) -> void
    {
        if (!_json_input.contains(prop_logical_path)) {
            THROW(INPUT_ARG_NOT_WELL_FORMED_ERR, fmt::format("[{}] is a required argument.", prop_logical_path));
        }

        throw_if_incorrect_type<std::string>(true, _json_input, prop_logical_path);
        throw_if_empty_string(true, _json_input, prop_logical_path);

        if (_json_input.contains(prop_options)) {
            if (!_json_input.at(prop_options.data()).is_object()) {
                THROW(INPUT_ARG_NOT_WELL_FORMED_ERR, fmt::format("[{}] must be a JSON object.", prop_options));
            }

            const auto& opts = _json_input.at(prop_options.data());
            const auto contains_no_create = opts.contains(prop_no_create);
            const auto contains_repl_num = opts.contains(prop_replica_number);
            const auto contains_leaf_resc_name = opts.contains(prop_leaf_resource_name);
            const auto contains_ref = opts.contains(prop_reference);
            const auto contains_secs_since_epoch = opts.contains(prop_seconds_since_epoch);

            if (contains_repl_num && contains_leaf_resc_name) {
                const auto* msg_fmt = "[{}] and [{}] cannot be used together.";
                THROW(USER_INCOMPATIBLE_PARAMS, fmt::format(msg_fmt, prop_replica_number, prop_leaf_resource_name));
            }

            if (contains_ref && contains_secs_since_epoch) {
                const auto* msg_fmt = "[{}] and [{}] cannot be used together.";
                THROW(USER_INCOMPATIBLE_PARAMS, fmt::format(msg_fmt, prop_reference, prop_seconds_since_epoch));
            }

            throw_if_incorrect_type<bool>(contains_no_create, opts, prop_no_create);
            throw_if_incorrect_type<replica_number_type>(contains_repl_num, opts, prop_replica_number);
            throw_if_incorrect_type<std::string>(contains_leaf_resc_name, opts, prop_leaf_resource_name);
            throw_if_incorrect_type<std::string>(contains_ref, opts, prop_reference);
            throw_if_incorrect_type<int>(contains_secs_since_epoch, opts, prop_seconds_since_epoch);

            throw_if_less_than_zero<replica_number_type>(contains_repl_num, opts, prop_replica_number);
            throw_if_empty_string(contains_leaf_resc_name, opts, prop_leaf_resource_name);
            throw_if_empty_string(contains_ref, opts, prop_reference);
            throw_if_less_than_zero<int>(contains_secs_since_epoch, opts, prop_seconds_since_epoch);
        }
    } // throw_if_input_is_invalid

    auto get_time(rsComm_t& _comm, const json& _json_input) -> fs::object_time_type
    {
        using duration_type = fs::object_time_type::duration;

        if (_json_input.contains(prop_options)) {
            const auto& opts = _json_input.at(prop_options.data());

            if (opts.contains(prop_reference)) {
                return fs::server::last_write_time(_comm, opts.at(prop_reference.data()).get_ref<const std::string&>());
            }

            if (opts.contains(prop_seconds_since_epoch)) {
                return fs::object_time_type{duration_type{opts.at(prop_seconds_since_epoch.data()).get<rep_type>()}};
            }
        }

        return std::chrono::time_point_cast<duration_type>(fs::object_time_type::clock::now());
    } // get_time

    // A return value of true indicates that the request has been handled entirely and the API plugin
    // should return immediately. This is the case where "no_create" is set to true.
    auto create_data_object_if_necessary(rsComm_t& _comm,
                                         const json& _json_input,
                                         const fs::path& _path) -> bool
    {
        namespace io = irods::experimental::io;

        if (fs::server::exists(_comm, _path)) {
            return false;
        }

        if (_json_input.contains(prop_options)) {
            const auto& opts = _json_input.at(prop_options.data());

            // Return immediately if the object does not exist and the user specified to not
            // create any new data objects.
            if (opts.contains(prop_no_create) && opts.at(prop_no_create.data()).get<bool>()) {
                return true;
            }

            if (opts.contains(prop_replica_number)) {
                THROW(OBJ_PATH_DOES_NOT_EXIST, "Replica numbers cannot be used when creating new data objects.");
            }

            // Create the data object at the leaf resource if one is provided.
            if (opts.contains(prop_leaf_resource_name)) {
                const auto& resc_name = opts.at(prop_leaf_resource_name.data()).get_ref<const std::string&>();
                bool is_coord_resc = false;

                if (const auto err = resc_mgr.is_coordinating_resource(resc_name.data(), is_coord_resc); !err.ok()) {
                    THROW(err.code(), err.result());
                }

                // Leaf resources cannot be coordinating resources. This essentially checks
                // if the resource has any child resources which is exactly what we're interested in.
                if (is_coord_resc) {
                    THROW(USER_INVALID_RESC_INPUT, fmt::format("[{}] is not a leaf resource.", resc_name));
                }

                io::server::default_transport tp{_comm};
                io::odstream{tp, _path, io::leaf_resource_name{resc_name}};

                return false;
            }
        }

        // Create a data object at the default resource (defined by the server). See the following for
        // more information.
        //
        //    https://docs.irods.org/master/system_overview/configuration/#default-resource-configuration
        //
        io::server::default_transport tp{_comm};
        io::odstream{tp, _path};

        return false;
    } // create_data_object_if_necessary

    auto get_replica_number(rsComm_t& _comm,
                            const json& _json_input,
                            const fs::path& _path) -> replica_number_type
    {
        if (_json_input.contains(prop_options)) {
            const auto& opts = _json_input.at(prop_options.data());

            // Return the replica number passed by the user.
            if (opts.contains(prop_replica_number)) {
                const auto replica_number = opts.at(prop_replica_number.data()).get<replica_number_type>();

                if (!ix::replica::replica_exists<rsComm_t>(_comm, _path, replica_number)) {
                    THROW(SYS_REPLICA_DOES_NOT_EXIST, "Replica does not exist matching that replica number.");
                }

                return replica_number;
            }

            // Convert the passed resource to a replica number and return it.
            if (opts.contains(prop_leaf_resource_name)) {
                const auto& resc_name = opts.at(prop_leaf_resource_name.data()).get_ref<const std::string&>();
                const auto replica_number = ix::replica::to_replica_number<rsComm_t>(_comm, _path, resc_name);

                if (!replica_number) {
                    THROW(SYS_REPLICA_DOES_NOT_EXIST, "Replica does not exist in resource.");
                }

                return *replica_number;
            }
        }

        // The user did not specify a target replica, so fetch the replica number
        // of the latest good replica.

        ix::query_builder qb;

        if (const auto zone = fs::zone_name(_path); zone) {
            qb.zone_hint(*zone);
        }

        // Fetch only good replicas (i.e. DATA_REPL_STATUS = '1').
        const auto gql = fmt::format("select DATA_MODIFY_TIME, DATA_REPL_NUM "
                                     "where"
                                     " COLL_NAME = '{}' and"
                                     " DATA_NAME = '{}' and"
                                     " DATA_REPL_STATUS = '1'",
                                     _path.parent_path().c_str(),
                                     _path.object_name().c_str());

        auto query = qb.build<rsComm_t>(_comm, gql);

        if (query.size() == 0) {
            THROW(SYS_NO_GOOD_REPLICA, "No good replicas found.");
        }

        std::uintmax_t latest_mtime = 0;
        replica_number_type replica_number = -1;

        for (auto&& row : query) {
            const auto mtime = std::stoull(row[0]);

            if (mtime > latest_mtime) {
                latest_mtime = mtime;
                replica_number = std::stoi(row[1]);
            }
        }

        return replica_number;
    } // get_replica_number

    auto rs_touch(rsComm_t* _comm, bytesBuf_t* _bbuf_input) -> int
    {
        try {
            namespace ic = irods::experimental::catalog;

            const auto json_input = parse_json(_bbuf_input);

            throw_if_input_is_invalid(json_input);

            // At this point, the input is guaranteed to be in a valid state.
            // The options should lead to a catalog update if the user has the
            // correct permissions.

            const fs::path path = json_input.at(prop_logical_path.data()).get_ref<const std::string&>();

            // This function will return true if the target data object does not exist and "no_create" is
            // set to true in the JSON input.
            if (const auto return_now = create_data_object_if_necessary(*_comm, json_input, path); return_now) {
                return 0;
            }

            //
            // The target object exists. Update the mtime.
            //

            const auto object_status = fs::server::status(*_comm, path);
            const auto is_collection = fs::server::is_collection(object_status);
            const auto is_data_object = fs::server::is_data_object(object_status);

            if (!is_collection && !is_data_object) {
                THROW(CAT_NOT_A_DATAOBJ_AND_NOT_A_COLLECTION, "Logical path does not point to a collection or data object.");
            }

            const auto new_mtime = get_time(*_comm, json_input);

            if (is_collection) {
                if (json_input.contains(prop_options)) {
                    const auto& opts = json_input.at(prop_options.data());

                    // Return immediately if invalid options are present in regards to the target.
                    if (opts.contains(prop_replica_number) || opts.contains(prop_leaf_resource_name)) {
                        const auto* msg_fmt = "[{}] and [{}] cannot be used for collections.";
                        THROW(USER_INCOMPATIBLE_PARAMS, fmt::format(msg_fmt, prop_replica_number, prop_leaf_resource_name));
                    }
                }

                if (!fs::server::is_collection_registered(*_comm, path)) {
                    THROW(CAT_NO_ROWS_FOUND, "Cannot update mtime (no catalog entry exists for collection).");
                }

                fs::server::last_write_time(*_comm, path, new_mtime);
            }
            else if (is_data_object) {
                if (!fs::server::is_data_object_registered(*_comm, path)) {
                    THROW(CAT_NO_ROWS_FOUND, "Cannot update mtime (no catalog entry exists for data object).");
                }

                const auto replica_number = get_replica_number(*_comm, json_input, path);
                ix::replica::last_write_time<rsComm_t>(*_comm, path, replica_number, new_mtime);
            }

            return 0;
        }
        catch (const fs::filesystem_error& e) {
            rodsLog(LOG_ERROR, e.what());
            addRErrorMsg(&_comm->rError, e.code().value(), e.what());
            return e.code().value();
        }
        catch (const irods::exception& e) {
            rodsLog(LOG_ERROR, e.what());
            addRErrorMsg(&_comm->rError, e.code(), e.client_display_what());
            return e.code();
        }
        catch (const std::exception& e) {
            rodsLog(LOG_ERROR, e.what());
            addRErrorMsg(&_comm->rError, SYS_UNKNOWN_ERROR, "Cannot process request due to an unexpected error.");
            return SYS_UNKNOWN_ERROR;
        }
    } // rs_touch

    auto call_touch(irods::api_entry* _api, rsComm_t* _comm, bytesBuf_t* _input) -> int
    {
        return _api->call_handler<bytesBuf_t*>(_comm, _input);
    } // call_touch

    using operation = std::function<int(rsComm_t*, bytesBuf_t*)>;
    const operation op = rs_touch;
    #define CALL_TOUCH call_touch
} // anonymous namespace

#else // RODS_SERVER

//
// Client-side Implementation
//

namespace
{
    using operation = std::function<int(rsComm_t*, bytesBuf_t*)>;
    const operation op{};
    #define CALL_TOUCH nullptr
} // anonymous namespace

#endif // RODS_SERVER

// The plugin factory function must always be defined.
extern "C"
auto plugin_factory(const std::string& _instance_name,
                    const std::string& _context) -> irods::api_entry*
{
#ifdef RODS_SERVER
    irods::client_api_whitelist::instance().add(TOUCH_APN);
#endif // RODS_SERVER

    // clang-format off
    irods::apidef_t def{TOUCH_APN,              // API number
                        RODS_API_VERSION,       // API version
                        NO_USER_AUTH,           // Client auth
                        NO_USER_AUTH,           // Proxy auth
                        "BinBytesBuf_PI", 0,    // In PI / bs flag
                        nullptr, 0,             // Out PI / bs flag
                        op,                     // Operation
                        "api_touch",            // Operation name
                        nullptr,                // Clear function
                        (funcPtr) CALL_TOUCH};
    // clang-format on

    auto* api = new irods::api_entry{def};

    api->in_pack_key = "BinBytesBuf_PI";
    api->in_pack_value = BytesBuf_PI;

    return api;
}

