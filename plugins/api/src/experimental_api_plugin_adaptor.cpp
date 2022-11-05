#include "irods/apiHandler.hpp"
#include "irods/plugins/api/api_plugin_number.h"
#include "irods/experimental_plugin_framework.hpp"
#include "irods/client_api_allowlist.hpp"
#include <nlohmann/json.hpp>

#include "irods/irods_re_namespaceshelper.hpp"
#include "irods/irods_re_plugin.hpp"
#include "irods/irods_re_ruleexistshelper.hpp"
#include "irods/irods_at_scope_exit.hpp"

#ifdef RODS_SERVER
#include "irods/plugin_lifetime_manager.hpp"
#endif

namespace {
    namespace ixa = irods::experimental::api;

    auto is_error(const json& j)
    {
        if(j.contains(ixa::constants::errors)) {
            for(auto& e : j.at(ixa::constants::errors)) {
                if(e.contains(ixa::constants::code)) {
                    auto c = e.at(ixa::constants::code);
                    // is acceptable error
                    if(c != SYS_NO_HANDLER_REPLY_MSG) {
                        return c.get<int>();
                    }
                }
            }
        }

        return 0;
    }

    auto to_json(bytesBuf_t* b)
    {
        const auto* buf = static_cast<char*>(b->buf);
        return json::parse(buf, buf + b->len);
    } // to_json

    auto to_bbuf(const json& j)
    {
        auto s = j.dump();
        auto b = new bytesBuf_t;
        b->buf = new char[s.size()+1];
        b->len = s.size();
        rstrcpy((char*)b->buf, s.c_str(), s.size()+1);
        return b;
    }

    auto error_to_bbuf(irods::error e)
    {
        return to_bbuf(json{{ixa::constants::errors, {
                            {ixa::constants::code, e.code()},
                            {ixa::constants::message, e.result()}}}});
    }

    using rule_engine_context_manager_type = irods::rule_engine_context_manager<irods::unit, ruleExecInfo_t*, irods::AUDIT_RULE>;

    template<typename... types_t>
    irods::error invoke_policy_enforcement_point(
        rule_engine_context_manager_type _re_ctx_mgr,
        irods::plugin_context&           _ctx,
        const std::string&               _operation_name,
        const std::string&               _class,
        types_t...                       _t)
    {
        using log = irods::experimental::log::rule_engine;

        bool ret = false;
        irods::error saved_op_err = SUCCESS();
        irods::error skip_op_err  = SUCCESS();

        for (auto& ns : NamespacesHelper::Instance()->getNamespaces()) {
            std::string rule_name = ns + "pep_api_" + _operation_name + "_" + _class;

            if (RuleExistsHelper::Instance()->checkOperation( rule_name ) ) {
                if (_re_ctx_mgr.rule_exists(rule_name, ret).ok() && ret) {
                    irods::error op_err = _re_ctx_mgr.exec_rule(
                                              rule_name,
                                              "experimental_api_plugin_adapter",
                                              _ctx,
                                              std::forward<types_t>(_t)...);

                    if (!op_err.ok()) {
                        log::debug("{}-pep rule [{}] failed with error code [{}]", _class, rule_name, op_err.code());
                        saved_op_err = op_err;
                    }
                    else if (op_err.code() == RULE_ENGINE_SKIP_OPERATION) {
                        skip_op_err = op_err;

                        if (_class != "pre") {
                            log::warn("RULE_ENGINE_SKIP_OPERATION ({}) incorrectly returned from PEP [{}]! "
                                      "RULE_ENGINE_SKIP_OPERATION should only be returned from pre-PEPs!",
                                      RULE_ENGINE_SKIP_OPERATION, rule_name);
                        }
                    }
                }
                else {
                    log::trace("Rule [{}] passes regex test, but does not exist", rule_name);
                }
            }
        }

        if (!saved_op_err.ok()) {
            return saved_op_err;
        }

        if (skip_op_err.code() == RULE_ENGINE_SKIP_OPERATION) {
            return skip_op_err;
        }

        return saved_op_err;
    } // invoke_policy_enforcement_point

    bool parameters_are_invalid(rsComm_t* comm, bytesBuf_t* bb_req)
    {
        if(!comm || !bb_req || bb_req->len == 0) {
            rodsLog(
                LOG_ERROR,
                "experimental_api_adaptor - one or more null or empty parameters");
            return true;
        }

        return false;
    } // parameters_are_invalid

    int adapter(rsComm_t* comm, bytesBuf_t* bb_req, bytesBuf_t** bb_resp)
    {
#ifdef RODS_SERVER
        const std::string plugin_type{"server"};

        if(parameters_are_invalid(comm, bb_req)) {
            return SYS_INVALID_INPUT_PARAM;
        }

        try {
            json resp{};

            auto obj = to_json(bb_req);

            auto request_dump = obj.dump();

            auto request = ixa::get<std::string>(ixa::commands::request, obj);

            auto plugin_name = ixa::get<std::string>(ixa::constants::plugin, obj);

            auto& plugins = ixa::plugin_lifetime_manager::instance().plugins();

            if(plugins.end() == plugins.find(plugin_name)) {
                plugins[plugin_name] = std::unique_ptr<ixa::base>(ixa::resolve_api_plugin(plugin_name, plugin_type));
            }

            irods::plugin_property_map prop_map;
            irods::plugin_context ctx(comm, prop_map);

            ruleExecInfo_t rei;
            memset( &rei, 0, sizeof( rei ) );
            if (comm) {
                rei.rsComm = comm;
                rei.uoic   = &comm->clientUser;
                rei.uoip   = &comm->proxyUser;
            }

            rule_engine_context_manager_type re_ctx_mgr(irods::re_plugin_globals->global_re_mgr, &rei);

            // Always run the finally-PEP at scope exit.
            irods::at_scope_exit invoke_finally_pep{[&] {
                irods::error finally_err = invoke_policy_enforcement_point(
                                               re_ctx_mgr,
                                               ctx,
                                               plugin_name,
                                               "finally",
                                               request_dump);

                if (!finally_err.ok()) {
                    irods::log(PASS(finally_err));
                }
            }};

            // invoke the pre-pep for this operation
            irods::error pre_err = invoke_policy_enforcement_point(
                                      re_ctx_mgr,
                                      ctx,
                                      plugin_name,
                                      "pre",
                                      request_dump);

            if (pre_err.code() != RULE_ENGINE_SKIP_OPERATION) {
                if (!pre_err.ok()) {
                    // if the pre-pep fails, invoke the exception pep
                    irods::error except_err = invoke_policy_enforcement_point(
                                                  re_ctx_mgr,
                                                  ctx,
                                                  plugin_name,
                                                  "except",
                                                  request_dump);

                    if (!except_err.ok()) {
                        irods::log(PASS(except_err));
                    }

                    *bb_resp = error_to_bbuf(pre_err);
                    return pre_err.code();
                }

                // invoke the actual api plugin
                resp = plugins[plugin_name]->call(comm, request, obj);

                auto error_code = is_error(resp);

                if (error_code < 0) {
                    // if the operation fails, invoke the exception pep
                    irods::error except_err = invoke_policy_enforcement_point(
                                                  re_ctx_mgr,
                                                  ctx,
                                                  plugin_name,
                                                  "except",
                                                  request_dump);

                    if (!except_err.ok()) {
                        irods::log(PASS(except_err));
                    }

                    *bb_resp = to_bbuf(resp);
                    return error_code;
                }

                // invoke the post-pep for this operation
                irods::error post_err = invoke_policy_enforcement_point(
                                            re_ctx_mgr,
                                            ctx,
                                            plugin_name,
                                            "post",
                                            request_dump);
                if (!post_err.ok()) {
                    // if the post-pep fails, invoke the exception pep
                    irods::error except_err = invoke_policy_enforcement_point(
                                                  re_ctx_mgr,
                                                  ctx,
                                                  plugin_name,
                                                  "except",
                                                  request_dump);

                    if (!except_err.ok()) {
                        irods::log(PASS(except_err));
                    }

                    *bb_resp = error_to_bbuf(post_err);
                    return post_err.code();
                }

            } // if not skip operation

            *bb_resp = to_bbuf(resp);
        }
        catch(const irods::exception& e) {
            rodsLog(LOG_ERROR, e.what());
            *bb_resp = to_bbuf(json{{ixa::constants::errors, {
                                    {ixa::constants::code, e.code()},
                                    {ixa::constants::message, e.what()}}}});
            return e.code();
        }
#endif

        return 0;

    } // adapter



    int call_adaptor(
          irods::api_entry* api
        , rsComm_t*         comm
        , bytesBuf_t*       inp
        , bytesBuf_t**      out)
    {
        return api->call_handler<bytesBuf_t*, bytesBuf_t**>(comm, inp, out);
    }



#ifdef RODS_SERVER
    #define CALL_ADAPTOR call_adaptor
#else
    #define CALL_ADAPTOR NULL
#endif

} // namespace

extern "C" {
    irods::api_entry* plugin_factory(const std::string&, const std::string&)
    {
#ifdef RODS_SERVER
        irods::client_api_allowlist::add(ADAPTER_APN);
#endif

        // =-=-=-=-=-=-=-
        // create a api def object
        irods::apidef_t def{ADAPTER_APN,                // api number
                            RODS_API_VERSION,           // api version
                            NO_USER_AUTH,               // client auth
                            NO_USER_AUTH,               // proxy auth
                            "BinBytesBuf_PI", 0,        // in PI / bs flag
                            "BinBytesBuf_PI", 0,        // out PI / bs flag
                            std::function<int(rsComm_t*, bytesBuf_t*, bytesBuf_t**)>(adapter),  // operation
                            "experimental_api_adaptor", // operation name
                            nullptr,                    // clear fcn
                            (funcPtr)CALL_ADAPTOR};
        return new irods::api_entry(def);
    } // plugin_factory

}; // extern "C"
