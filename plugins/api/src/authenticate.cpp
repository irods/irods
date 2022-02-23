#include "irods/authentication_plugin_framework.hpp"

#include "irods/apiHandler.hpp"
#include "irods/plugins/api/api_plugin_number.h"
#include "irods/irods_logger.hpp"
#include "irods/rodsPackInstruct.h"

#include <nlohmann/json.hpp>

#ifdef RODS_SERVER

#include "irods/client_api_whitelist.hpp"
#include "irods/json_serialization.hpp"
#include "irods/server_utilities.hpp"

namespace
{
    template<typename T>
    auto get(const std::string& n, const json& p)
    {
        if(!p.contains(n)) {
            THROW(SYS_INVALID_INPUT_PARAM, fmt::format(
				  "authentication request missing [{}] parameter", n));
        }

        return p.at(n).get<T>();
    } // get

    // Takes a BytesBuf and reinterprets it as a string which is then parsed as JSON and returned.
    auto to_json(BytesBuf* _p) -> nlohmann::json
    {
        if (!_p) {
            return nullptr;
        }

        return json::parse((char*)_p->buf, (char*)_p->buf+_p->len);
    } // to_json

    int authenticate(rsComm_t* comm, bytesBuf_t* bb_req, bytesBuf_t** bb_resp)
    {
        namespace ie = irods::experimental;

        if (!comm || !bb_req || bb_req->len == 0) {
            ie::log::api::info("one or more null parameters");
            return SYS_INVALID_INPUT_PARAM;
        }

        try {
            auto req  = to_json(bb_req);
            auto auth = ie::resolve_authentication_plugin(
                                get<std::string>("scheme", req), "server");
            auto opr  = get<std::string>("next_operation", req);
            auto resp = auth->call(*comm, opr, req);

            *bb_resp = irods::to_bytes_buffer(resp.dump());
        }
        catch (const irods::exception& e) {
            const std::string msg = fmt::format(
                "Error occurred invoking auth plugin operation [{}] [ec={}]",
                e.client_display_what(), e.code());

            ie::log::api::info(msg);
            ie::log::api::debug(e.what());

            return e.code();
        }
        catch (const std::exception& e) {
            const std::string msg = fmt::format(
                "Error occurred invoking auth plugin operation [{}]",
                e.what());

            ie::log::api::info(msg);

            return SYS_INTERNAL_ERR;
        }
        catch (...) {
            const std::string msg = "Unknown error occurred invoking auth plugin operation.";

            ie::log::api::info(msg);

            return SYS_UNKNOWN_ERROR;
        }

        return 0;
    } // authenticate

    int call_authenticate(
          irods::api_entry* api
        , rsComm_t*         comm
        , bytesBuf_t*       inp
        , bytesBuf_t**      out)
    {
        return api->call_handler<bytesBuf_t*, bytesBuf_t**>(comm, inp, out);
    }

    using operation = std::function<int(rsComm_t*, bytesBuf_t*, bytesBuf_t**)>;
    const operation op = authenticate;
    #define CALL_AUTHENTICATE call_authenticate
} // anonymous namespace

#else // RODS_SERVER
//
// Client-side Implementation
//

namespace
{
    using operation = std::function<int(rsComm_t*, bytesBuf_t*, bytesBuf_t**)>;
    const operation op{};
    #define CALL_AUTHENTICATE NULL
}

#endif // RODS_SERVER

extern "C" {
    irods::api_entry* plugin_factory(const std::string&, const std::string&)
    {
#ifdef RODS_SERVER
        irods::client_api_whitelist::instance().add(AUTHENTICATION_APN);
#endif

        // =-=-=-=-=-=-=-
        // create a api def object
        irods::apidef_t def{AUTHENTICATION_APN, // api number
                            RODS_API_VERSION,   // api version
                            NO_USER_AUTH,       // client auth
                            NO_USER_AUTH,       // proxy auth
                            "BinBytesBuf_PI", 0,                        // In PI / bs flag
                            "BinBytesBuf_PI", 0,                        // Out PI / bs flag
                            op,                 // operation
                            "api_authenticate", // operation name
                            nullptr,            // null clear fcn
                            (funcPtr)CALL_AUTHENTICATE};

        auto* api = new irods::api_entry{def};

        api->in_pack_key = "BinBytesBuf_PI";
        api->in_pack_value = BytesBuf_PI;

        api->out_pack_key = "BinBytesBuf_PI";
        api->out_pack_value = BytesBuf_PI;

        return api;
    } // plugin_factory
} // extern "C"
