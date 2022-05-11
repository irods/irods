#ifndef IRODS_AUTHENTICATION_PLUGIN_FRAMEWORK_HPP
#define IRODS_AUTHENTICATION_PLUGIN_FRAMEWORK_HPP

#include "irods/authenticate.h"
#include "irods/getRodsEnv.h"
#include "irods/irods_auth_constants.hpp"
#include "irods/irods_exception.hpp"
#include "irods/irods_load_plugin.hpp"
#include "irods/irods_plugin_base.hpp"
#include "irods/rcConnect.h"
#include "irods/rodsErrorTable.h"
#include "irods/version.hpp"

#include <boost/any.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <vector>

/// \file

using json = nlohmann::json;

namespace irods::experimental::auth
{
    static const char* const flow_complete{"authentication_flow_complete"};
    static const char* const next_operation{"next_operation"};

    /// \brief Base class for authentication plugin implementations.
    ///
    /// \since 4.3.0
    class authentication_base : public irods::plugin_base
    {
    public:
        #define OPERATION(C, F) std::function<json(C&, const json&)>([&](C& c, const json& j) -> json {return F(c, j);})

        /// \brief Constructor for the authentication plugin base class.
        ///
        /// \since 4.3.0
        authentication_base()
            : plugin_base{"authentication_framework_plugin", "empty_context_string"}
        {
            add_operation(AUTH_CLIENT_START, OPERATION(RcComm, auth_client_start));
        } // ctor

        /// Pure virtual method which all authentication plugins must implement.
        ///
        /// \since 4.3.0
        virtual json auth_client_start(RcComm& _comm, const json& req) = 0;

        /// \brief Add the operation captured in \p f to the plugin's operation map at key \p n.
        ///
        /// \param[in] n Key with which the operation will be discovered in the operation map.
        /// \param[in] f Function object which implements the operation being added.
        ///
        /// \throws irods::exception If \p n is empty or already exists in the operation map.
        ///
        /// \since 4.3.0
        template<typename RxComm>
        void add_operation(const std::string& n, std::function<json(RxComm&, const json&)> f)
        {
            if(n.empty()) {
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format("operation name is empty [{}]", n));
            }

            if (operations_.has_entry(n)) {
                THROW(SYS_INTERNAL_ERR, fmt::format("operation already exists [{}]", n));
            }

            operations_[n] = f;
        } // add_operation

        /// \brief Invoke the operation at key \p n.
        ///
        /// param[in/out] _comm iRODS communication object.
        /// param[in] n Key associated with the operation being invoked.
        /// param[in] req JSON payload including the data for the operation.
        ///
        /// \throws irods::exception If there is no key \p n in the operations map.
        ///
        /// \returns JSON object representing the result of the operation.
        ///
        /// \since 4.3.0
        template<typename RxComm>
        json call(RxComm& _comm, const std::string& n, const json& req)
        {
            if (!operations_.has_entry(n)) {
                THROW(SYS_INVALID_INPUT_PARAM,
                      fmt::format("call operation :: missing operation[{}]", n));
            }

            using fcn_t = std::function<json(RxComm&, const json&)>;
            auto op = boost::any_cast<fcn_t&>(operations_[n]);

            return op(_comm, req);
        } // call
    }; // class authentication_base

    /// \brief Resolve the authentication plugin with \p _scheme of \p _type "client" or "server".
    ///
    /// \param[in] _scheme The authentication scheme whose plugin is being loaded.
    /// \param[in] _type A string which indicates the type of the plugin ("client" or "server").
    ///
    /// \throws irods::exception If loading the plugin is unsuccessful.
    ///
    /// \returns The authentication plugin object (descendant of \p authentication_base).
    ///
    /// \since 4.3.0
    auto resolve_authentication_plugin(const std::string& _scheme, const std::string& _type)
    {
        using plugin_type = authentication_base;

        std::string scheme = _scheme;
        std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);

        const std::string name = fmt::format("irods_auth_plugin-{}_{}", scheme, _type);

        plugin_type* plugin{};
        auto err = irods::load_plugin<plugin_type>(plugin,
                                                   name,
                                                   irods::KW_CFG_PLUGIN_TYPE_AUTHENTICATION,
                                                   name,
                                                   "empty_context");
        if(!err.ok()) {
            THROW(err.code(), err.result());
        }

        return plugin;
    } // resolve_authentication_plugin

    /// \brief Authenticate the client indicated by \p _comm with scheme \p _env.
    ///
    /// \parblock
    /// Starting with the operation indicated by \p irods::AUTH_CLIENT_START in the operation
    /// map of the authentication plugin, this function loops until the response from an
    /// invoked operation in the plugin returns a "next_operation" of \p auth::flow_complete.
    /// At that time, \p _comm.loggedIn should be set to 1 (or anything other than 0) and the
    /// client is considered authenticated.
    ///
    /// This function acts as the interface to the authentication plugins much like
    /// \p clientLogin was the interface for the legacy authentication plugins.
    /// \endparblock
    ///
    /// \param[in/out] _comm iRODS communication object.
    /// \param[in] _env Environment object from which the authentication scheme is retrieved.
    /// \param[in] _ctx JSON object which includes information for the authentication plugin.
    ///
    /// \throws irods::exception \parblock
    /// - If the authentication plugin cannot be resolved
    /// - If an operation fails to set "next_operation" to a non-empty string in its response
    /// - If the flow is completed with \p !_comm.loggedIn
    /// \endparblock
    ///
    /// \since 4.3.0
    void authenticate_client(RcComm& _comm, const RodsEnvironment& _env, const json& _ctx)
    {
        std::string scheme = _env.rodsAuthScheme;

        auto auth = resolve_authentication_plugin(scheme, "client");

        const std::string* next_operation = &irods::AUTH_CLIENT_START;

        json req{_ctx}, resp{};

        req["scheme"] = scheme;
        req[auth::next_operation] = *next_operation;

        while (true) {
            resp = auth->call(_comm, *next_operation, req);

            if (_comm.loggedIn) {
                break;
            }

            if (!resp.contains(auth::next_operation)) {
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format(
                      "authentication request missing [{}] parameter",
                      auth::next_operation));
            }

            next_operation = resp.at(auth::next_operation).get_ptr<std::string*>();
            if (next_operation->empty() || auth::flow_complete == *next_operation) {
                THROW(CAT_INVALID_AUTHENTICATION,
                      "authentication flow completed without success");
            }

            req = resp;
        }
    } // authenticate_client

    /// \brief Return true if server version is too old for authentication plugin framework.
    ///
    /// \param[in] _comm The comm object from which version information is derived.
    ///
    /// \retval true If server version is less than 4.3.0
    /// \retval false If server version is greater than or equal to 4.3.0
    ///
    /// \since 4.3.0
    auto use_legacy_authentication(const RcComm& _comm) -> bool
    {
        static constexpr auto minimum_version_for_auth_plugin_framework = irods::version{4, 3, 0};

        const auto server_version = irods::to_version(_comm.svrVersion->relVersion);
        if (!server_version) {
            THROW(VERSION_EMPTY_IN_STRUCT_ERR, fmt::format(
                  "Failed to get version from server [{}]\n",
                  _comm.svrVersion->relVersion));
        }

        return *server_version < minimum_version_for_auth_plugin_framework;
    } // use_legacy_authentication

    /// \brief Convenience function for invoking the authentication API endpoint.
    ///
    /// \param[in/out] _comm
    /// \param[in] _msg JSON-based request message to send to the server.\parblock
    ///
    /// Depending on the step in the authentication flow for the given plugin, certain keys
    /// must be present in order for the request to be processed.
    /// \endparblock
    ///
    /// \throws irods::exception If the API request returns with a non-zero code
    ///
    /// \return JSON response from the server which may be built upon for the next request
    ///
    /// \since 4.3.0
    auto request(rcComm_t& _comm, const json& _msg)
    {
        constexpr int authentication_api_number = 110000;

        auto str = _msg.dump();

        bytesBuf_t inp;
        inp.buf = static_cast<void*>(const_cast<char*>(str.c_str()));
        inp.len = str.size();

        bytesBuf_t* resp{};
        auto ec = procApiRequest(&_comm,
                                 authentication_api_number,
                                 static_cast<void*>(&inp),
                                 nullptr,
                                 reinterpret_cast<void**>(&resp),
                                 nullptr);

        if (ec < 0) {
            THROW(ec, "failed to perform request");
        }

        return json::parse(static_cast<char*>(resp->buf),
                           static_cast<char*>(resp->buf) + resp->len);
    } // request

    /// \brief Throw if request message is missing a required key
    ///
    /// \param[in] _msg JSON structure in which \p _required_keys must be found
    /// \param[in] _required_keys Vector of keys which must be found in \p _msg
    ///
    /// \throw irods::exception On first key in \p _required_keys not contained in \p _msg
    ///
    /// \since 4.3.0
    inline auto throw_if_request_message_is_missing_key(
        const json& _msg,
        const std::vector<std::string_view>& _required_keys) -> void
    {
        for (const auto& k : _required_keys) {
            if (!_msg.contains(k.data())) {
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format(
                      "missing [{}] in request", __func__, __LINE__, k));
            }
        }
    } // throw_if_request_message_is_missing_key
} // namespace irods::experimental::auth

#endif // IRODS_AUTHENTICATION_PLUGIN_FRAMEWORK_HPP
