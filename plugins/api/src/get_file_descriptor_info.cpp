#include "api_plugin_number.h"
#include "rodsDef.h"
#include "rcConnect.h"
#include "rodsPackInstruct.h"

#include "apiHandler.hpp"

#include <functional>

#if defined(RODS_SERVER) || defined(RODS_CLERVER)

//
// Server-side Implementation
//

#include "objDesc.hpp"
#include "irods_stacktrace.hpp"
#include "irods_server_api_call.hpp"
#include "irods_re_serialization.hpp"
#include "irods_get_l1desc.hpp"
#include "irods_logger.hpp"

#include <string>
#include <tuple>

#include "json.hpp"

namespace
{
    // clang-format off
    using json      = nlohmann::json;
    using operation = std::function<int(rsComm_t*, bytesBuf_t*, bytesBuf_t**)>;
    // clang-format on

    //
    // Function Prototypes
    //

    auto to_json(const specColl_t*) -> json;
    auto to_json(const keyValPair_t&) -> json;
    auto to_json(const dataObjInp_t*) -> json;
    auto to_json(const dataObjInfo_t*) -> json;
    auto to_json(const l1desc&) -> json;

    auto call_get_file_descriptor_info(irods::api_entry*, rsComm_t*, bytesBuf_t*, bytesBuf_t**) -> int;

    auto is_input_valid(const bytesBuf_t*) -> std::tuple<bool, std::string>;

    auto get_file_descriptor(const bytesBuf_t& _buf) -> int;

    auto rs_get_file_descriptor_info(rsComm_t*, bytesBuf_t*, bytesBuf_t**) -> int;

    //
    // Function Implementations
    //

    auto to_json(const specColl_t* _p) -> json
    {
        if (!_p) {
            return nullptr;
        }

        return {
            {"collection_class", static_cast<int>(_p->collClass)},
            {"type", static_cast<int>(_p->type)},
            {"collection", _p->collection},
            {"object_path", _p->objPath},
            {"resource", _p->resource},
            {"resource_hierarchy", _p->rescHier},
            {"physical_path", _p->phyPath},
            {"cache_directory", _p->cacheDir},
            {"is_cache_dirty", static_cast<bool>(_p->cacheDirty)},
            {"replica_number", _p->replNum}
        };
    }

    auto to_json(const keyValPair_t& _conds) -> json
    {
        auto json_array = json::array();

        for (int i = 0; i < _conds.len; ++i) {
            json_array.push_back({{"key", _conds.keyWord[i]}, {"value", _conds.value[i]}});
        }

        return json_array;
    }

    auto to_json(const dataObjInp_t* _p) -> json
    {
        if (!_p) {
            return nullptr;
        }

        return {
            {"object_path", _p->objPath},
            {"create_mode", _p->createMode},
            {"open_flags", _p->openFlags},
            {"offset", _p->offset},
            {"data_size", _p->dataSize},
            {"number_of_threads", _p->numThreads},
            {"operation_type", _p->oprType},
            {"special_collection", to_json(_p->specColl)},
            {"condition_input", to_json(_p->condInput)}
        };
    }

    auto to_json(const dataObjInfo_t* _p) -> json
    {
        if (!_p) {
            return nullptr;
        }

        return {
            {"object_path", _p->objPath},
            {"resource_name", _p->rescName},
            {"resource_hierarchy", _p->rescHier},
            {"data_type", _p->dataType},
            {"data_size", _p->dataSize},
            {"checksum", _p->chksum},
            {"version", _p->version},
            {"file_path", _p->filePath},
            {"data_owner_name", _p->dataOwnerName},
            {"data_owner_zone", _p->dataOwnerZone},
            {"replica_number", _p->replNum},
            {"is_replica_current", (_p->replStatus == 1)},
            {"status_string", _p->statusString},
            {"data_id", _p->dataId},
            {"collection_id", _p->collId},
            {"data_map_id", _p->dataMapId},
            {"flags", _p->flags},
            {"data_comments", _p->dataComments},
            {"data_mode", _p->dataMode},
            {"data_expiry", _p->dataExpiry},
            {"data_create", _p->dataCreate},
            {"data_modify", _p->dataModify},
            {"data_access", _p->dataAccess},
            {"data_access_index", _p->dataAccessInx},
            {"write_flag", _p->writeFlag},
            {"destination_resource_name", _p->destRescName},
            {"backup_resource_name", _p->backupRescName},
            {"sub_path", _p->subPath},
            {"special_collection", to_json(_p->specColl)},
            {"registering_user_id", _p->regUid},
            {"other_flags", _p->otherFlags},
            {"condition_input", to_json(_p->condInput)},
            {"in_pdmo", _p->in_pdmo},
            {"next", to_json(_p->next)},
            {"resource_id", _p->rescId}
        };
    }

    auto to_json(const l1desc& _fd_info) -> json
    {
        return {
            {"l3descInx", _fd_info.l3descInx},
            {"in_use", static_cast<bool>(_fd_info.inuseFlag)},
            {"operation_type", _fd_info.oprType},
            {"open_type", _fd_info.openType},
            {"operation_status", _fd_info.oprStatus},
            {"data_object_input_replica_flag", _fd_info.dataObjInpReplFlag},
            {"data_object_input", to_json(_fd_info.dataObjInp)},
            {"data_object_info", to_json(_fd_info.dataObjInfo)},
            {"other_data_object_info", to_json(_fd_info.otherDataObjInfo)},
            {"copies_needed", _fd_info.copiesNeeded},
            {"bytes_written", _fd_info.bytesWritten},
            {"data_size", _fd_info.dataSize},
            {"replica_status", _fd_info.replStatus},
            {"checksum_flag", _fd_info.chksumFlag},
            {"source_l1_descriptor_index", _fd_info.srcL1descInx},
            {"checksum", _fd_info.chksum},
            {"remote_l1_descriptor_index", _fd_info.remoteL1descInx},
            {"stage_flag", _fd_info.stageFlag},
            {"purge_cache_flag", _fd_info.purgeCacheFlag},
            {"lock_file_descriptor", _fd_info.lockFd},
            {"plugin_data", nullptr}, // Not used anywhere as of 2019-01-28
            {"in_pdmo", _fd_info.in_pdmo}
        };
    }

    auto call_get_file_descriptor_info(irods::api_entry* _api,
                                       rsComm_t* _comm,
                                       bytesBuf_t* _input,
                                       bytesBuf_t** _output) -> int
    {
        return _api->call_handler<bytesBuf_t*, bytesBuf_t**>(_comm, _input, _output);
    }

    auto is_input_valid(const bytesBuf_t* _input) -> std::tuple<bool, std::string>
    {
        if (!_input) {
            return {false, "Missing JSON input"};
        }

        if (_input->len == 0) {
            return {false, "Length of buffer must be greater than zero"};
        }

        if (!_input->buf) {
            return {false, "Missing input buffer"};
        }

        return {true, ""};
    }

    auto get_file_descriptor(const bytesBuf_t& _buf) -> int
    {
        return json::parse(std::string(static_cast<const char*>(_buf.buf), _buf.len)).at("fd").get<int>();
    }

    auto to_bytes_buffer(const std::string& _s) -> bytesBuf_t*
    {
        char* buf = new char[_s.length() + 1]{};
        std::strncpy(buf, _s.c_str(), _s.length());

        bytesBuf_t* bbp = new bytesBuf_t{};
        bbp->len = _s.length();
        bbp->buf = buf;

        return bbp;
    }

    auto rs_get_file_descriptor_info(rsComm_t* _comm, bytesBuf_t* _input, bytesBuf_t** _output) -> int
    {
        using log = irods::experimental::log;

        if (const auto [valid, msg] = is_input_valid(_input); !valid) {
            log::api::error(msg);
            return 0;
        }

        int fd = -1;

        try {
            fd = get_file_descriptor(*_input);
        }
        catch (const json::type_error& e) {
            // clang-format off
            log::api::error({{"log_message", "Failed to extract file descriptor from input"},
                             {"error_message", e.what()}});
            // clang-format on
            return 0;
        }

        *_output = to_bytes_buffer(to_json(irods::get_l1desc(fd)).dump());

        return 0;
    }

    const operation op = rs_get_file_descriptor_info;
    #define CALL_GET_FD_INFO call_get_file_descriptor_info
} // anonymous namespace

#else // defined(RODS_SERVER) || defined(RODS_CLERVER)

//
// Client-side Implementation
//

#include "get_file_descriptor_info.h"
#include "procApiRequest.h"

#include <cstring>

namespace
{
    using operation = std::function<int(rsComm_t*, bytesBuf_t*, bytesBuf_t**)>;
    const operation op{};
    #define CALL_GET_FD_INFO nullptr
} // anonymous namespace

extern "C"
auto rc_get_file_descriptor_info(rcComm_t* _comm,
                                 const char* _json_input,
                                 char** _json_output) -> int
{
    if (!_json_input) {
        return -1;
    }

    bytesBuf_t input_buf{};
    input_buf.len = static_cast<int>(std::strlen(_json_input));
    input_buf.buf = const_cast<char*>(_json_input);

    bytesBuf_t* output_buf{};

    const int ec = procApiRequest(_comm, GET_FILE_DESCRIPTOR_INFO_APN,
                                  &input_buf, nullptr,
                                  reinterpret_cast<void**>(&output_buf), nullptr);

    if (ec == 0) {
        *_json_output = static_cast<char*>(output_buf->buf);
    }

    return ec;
}

#endif // defined(RODS_SERVER) || defined(RODS_CLERVER)

// The plugin factory function must always be defined.
extern "C"
auto plugin_factory(const std::string& _instance_name,
                    const std::string& _context) -> irods::api_entry*
{
    // clang-format off
    irods::apidef_t def{GET_FILE_DESCRIPTOR_INFO_APN,               // API number
                        RODS_API_VERSION,                           // API version
                        NO_USER_AUTH,                               // Client auth
                        NO_USER_AUTH,                               // Proxy auth
                        "BytesBuf_PI", 0,                           // In PI / bs flag
                        "BytesBuf_PI", 0,                           // Out PI / bs flag
                        op,                                         // Operation
                        "rs_get_file_descriptor_info",              // Operation name
                        nullptr,                                    // Null clear function
                        (funcPtr) CALL_GET_FD_INFO};
    // clang-format on

    auto* api = new irods::api_entry{def};

    api->in_pack_key = "BytesBuf_PI";
    api->in_pack_value = BytesBuf_PI;

    api->out_pack_key = "BytesBuf_PI";
    api->out_pack_value = BytesBuf_PI;

    return api;
}

