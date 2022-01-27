#ifndef IRODS_AUTHENTICATION_PLUGIN_FRAMEWORK_HPP
#define IRODS_AUTHENTICATION_PLUGIN_FRAMEWORK_HPP

#include "authCheck.h"
#include "authPluginRequest.h"
#include "authRequest.h"
#include "authResponse.h"
#include "authenticate.h"
#include "irods_auth_constants.hpp"
#include "irods_auth_factory.hpp"
#include "irods_auth_manager.hpp"
#include "irods_auth_plugin.hpp"
#include "irods_kvp_string_parser.hpp"
#include "irods_native_auth_object.hpp"
#include "irods_stacktrace.hpp"
#include "miscServerFunct.hpp"
#include "msParam.h"
#include "rcConnect.h"
#include "rodsDef.h"

#ifdef RODS_SERVER
#include "rsAuthCheck.hpp"
#include "rsAuthRequest.hpp"
#endif // RODS_SERVER

#include <boost/any.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <openssl/md5.h>

#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>

using json = nlohmann::json;

namespace irods::experimental
{
    namespace auth
    {
        static const std::string flow_complete{"authentication_flow_complete"};

        static const std::string next_operation{"next_operation"};
    }

    class authentication_base : public irods::plugin_base
    {
    public:
        #define OPERATION(C, F) std::function<json(C&, const json&)>([&](C& c, const json& j) -> json {return F(c, j);})

        authentication_base()
            : plugin_base{"authentication_framework_plugin", "empty_context_string"}
        {
            add_operation(AUTH_CLIENT_START, OPERATION(rcComm_t, auth_client_start));
        } // ctor

        virtual json auth_client_start(rcComm_t& comm, const json& req) = 0;

        template<typename RxComm>
        void add_operation(const std::string& n, std::function<json(RxComm&, const json&)> f)
        {
            if(n.empty()) {
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format("operation name is empty [{}]", n));
            }

            if(operations_.find(n) != operations_.end()) {
                THROW(SYS_INTERNAL_ERR, fmt::format("operation already exists [{}]", n));
            }

            operations_[n] = f;
        } // add_operation

        template<typename RxComm>
        json call(RxComm& comm, const std::string& n, const json& req)
        {
            auto itr = operations_.find(n);
            if(itr == operations_.end()) {
                THROW(SYS_INVALID_INPUT_PARAM,
                      fmt::format("call operation :: missing operation[{}]", n));
            }

            using fcn_t = std::function<json(RxComm&, const json&)>;
            auto op = boost::any_cast<fcn_t&>(operations_[n]);

            return op(comm, req);
        } // call
    }; // class authentication_base

    auto resolve_authentication_plugin(const std::string& scheme, const std::string& type)
    {
        using plugin_type = irods::experimental::authentication_base;

        std::string lower = scheme;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        const std::string name = fmt::format("irods_auth_plugin-{}_{}", lower, type);

        plugin_type* plugin{};
        auto err = irods::load_plugin<plugin_type>(
                        plugin,
                        name,
                        irods::PLUGIN_TYPE_AUTHENTICATION,
                        name,
                        "empty_context");
        if(!err.ok()) {
            THROW(err.code(), err.result());
        }

        return plugin;
    } // resolve_authentication_plugin

    void authenticate_client(rcComm_t& comm, const rodsEnv& env)
    {
        // example native authentication scheme: irods-authentication_plugin-native
        std::string scheme{env.rodsAuthScheme};

        // TODO:: make some decisions about auth scheme?

        auto auth = resolve_authentication_plugin(scheme, "client");

        const std::string* next_operation = &irods::AUTH_CLIENT_START;

        json req{}, resp{};

        req["scheme"] = scheme;
        req[auth::next_operation] = *next_operation;

        while (true) {
            resp = auth->call(comm, *next_operation, req);

            if(comm.loggedIn) {
                break;
            }

            if(!resp.contains(auth::next_operation)) {
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format(
                      "authentication request missing [{}] parameter",
                      auth::next_operation));
            }

            next_operation = resp.at(auth::next_operation).get_ptr<std::string*>();
            if(next_operation->empty() || auth::flow_complete == *next_operation) {
                THROW(CAT_INVALID_AUTHENTICATION,
                      "authentication flow completed without success");
            }

            req = resp;
        }
    } // authenticate_client
} // namespace irods::experimental

#endif // IRODS_AUTHENTICATION_PLUGIN_FRAMEWORK_HPP
