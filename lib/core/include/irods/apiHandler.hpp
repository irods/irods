#ifndef IRODS_API_HANDLER_HPP
#define IRODS_API_HANDLER_HPP

#include "irods/rods.h"
#include "irods/packStruct.h"
#include "irods/irods_lookup_table.hpp"
#include "irods/irods_plugin_base.hpp"
#include "irods/irods_re_ruleexistshelper.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/irods_pack_table.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_re_namespaceshelper.hpp"
#include "irods/irods_re_plugin.hpp"
#include "irods/irods_re_ruleexistshelper.hpp"
#include "irods/irods_logger.hpp"
#include "irods/rcMisc.h"

#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>

#include <typeinfo>
#include <functional>
#include <utility>
#include <type_traits>
#include <vector>
#include <string>
#include <string_view>

namespace irods
{
    struct apidef_t {
        // =-=-=-=-=-=-=-
        // attributes
        int            apiNumber;      /* the API number */
        char*          apiVersion;     /* The API version of this call */
        int            clientUserAuth; /* Client user authentication level.
                                        * NO_USER_AUTH, REMOTE_USER_AUTH,
                                        * LOCAL_USER_AUTH, REMOTE_PRIV_USER_AUTH or
                                        * LOCAL_PRIV_USER_AUTH */
        int            proxyUserAuth;                    /* same for proxyUser */
        const char*    inPackInstruct; /* the packing instruct for the input
                                        * struct */
        int inBsFlag;                  /* input bytes stream flag. 0 ==> no input
                                        * byte stream. 1 ==> we have an input byte
                                        * stream */
        const char*    outPackInstruct;/* the packing instruction for the
                                        * output struct */
        int            outBsFlag;      /* output bytes stream. 0 ==> no output byte
                                        * stream. 1 ==> we have an output byte stream
                                        */
        boost::any     svrHandler;     /* the server handler. should be defined NULL for
                                        * client */

        const char *   operation_name;

        std::function<void(void*)> clearInStruct;	// free input struct function
        std::function<void(void*)> clearOutStruct;	// free output struct function

        int(*call_wrapper)(...);        // wraps the api call for type casting
    }; // struct apidef_t

    template <typename Integer,
              typename std::enable_if_t<std::is_integral<Integer>::value, int> = 0>
    bool is_acceptable_error(Integer error_code) {
        static const std::vector<Integer> acceptable_errors {
            SYS_NO_HANDLER_REPLY_MSG //... others as necessary
        };
        return std::any_of( acceptable_errors.begin(), acceptable_errors.end(),
            [error_code](auto listed)->bool { return listed == error_code; }
        );
    }

    template <typename... types_t>
    class api_call_adaptor {
    private:
        std::function<int(rsComm_t*, types_t...)> fcn_;

    public:
        api_call_adaptor( std::function<int(rsComm_t*, types_t...)> _fcn )
            : fcn_(_fcn)
        {
        }

        irods::error operator()( irods::plugin_context&, rsComm_t* _comm, types_t... _t )
        {
#ifdef IRODS_ENABLE_SYSLOG
            bool logger_updated = false;

            const auto update_logger = [&logger_updated](auto&& _arg)
            {
                // If there exists multiple objects containing verbose flags,
                // then only allow one update.  What happens if subsequent calls
                // are made (hopefully, the condition objects are passed through
                // unchanged)?
                if (logger_updated)
                {
                    return;
                }

                // _arg's type will be deduced as <type>& if it's an lvalue.
                // For pointers, this means the type will we T*&.  Therefore, we
                // must either remove the reference or add a reference so that
                // std::is_same compares the correct types.
                using T = std::remove_reference_t<decltype(_arg)>;

                if constexpr (std::is_same_v<dataObjInp_t*, T>)
                {
                    logger_updated = true;
                    const auto* value = getValByKey(&_arg->condInput, VERY_VERBOSE_KW);
                    irods::experimental::log::write_to_error_object(value); 
                }
                else if constexpr (std::is_same_v<dataObjCopyInp_t*, T>)
                {
                    logger_updated = true;
                    const auto* flag_0 = getValByKey(&_arg->srcDataObjInp.condInput, VERY_VERBOSE_KW);
                    const auto* flag_1 = getValByKey(&_arg->destDataObjInp.condInput, VERY_VERBOSE_KW);
                    irods::experimental::log::write_to_error_object(flag_0 && flag_1); 
                }
            };

            (update_logger(std::forward<types_t>(_t)), ...);
#endif // IRODS_ENABLE_SYSLOG

            int ret = fcn_( _comm, _t... );
            if( ret >= 0 ) {
                return CODE( ret );
            }
            else {
                return ERROR( ret, "fail" );
            }
        }

    }; // class api_call_adaptor

    class api_entry : public irods::plugin_base
    {
    private:
        using rule_engine_context_manager_type = rule_engine_context_manager<unit, ruleExecInfo_t*, AUDIT_RULE>;

    public:
        /// =-=-=-=-=-=-=-
        /// @brief adaptor from old api sig to new plugin sig
        template<typename... types_t>
        error add_operation(const std::string& _op, std::function<int(types_t...)> _f)
        {
            if ( _op.empty() ) {
                std::stringstream msg;
                msg << "empty operation key [" << _op << "]";
                return ERROR(SYS_INVALID_INPUT_PARAM, msg.str());
            }

            operation_name = _op;
            operations_[operation_name] = _f;

            return SUCCESS();
        } // add_operation

        template<typename... types_t>
        int call_handler(rsComm_t* _comm, types_t... _t)
        {
            using namespace std;

            if( !operations_.has_entry(operation_name) ) {
                rodsLog(LOG_ERROR, "missing api operation [%s]", operation_name.c_str());
                return SYS_INVALID_INPUT_PARAM;
            }

            try {
                using fcn_t = std::function<int(rsComm_t*, types_t...)>;
                fcn_t fcn = boost::any_cast<fcn_t>( operations_[ operation_name ] );
#ifdef ENABLE_RE
                irods::plugin_property_map prop_map;
                irods::plugin_context ctx(_comm,prop_map);
                ruleExecInfo_t rei;
                memset( &rei, 0, sizeof( rei ) );
                if (_comm) {
                    rei.rsComm      = _comm;
                    rei.uoic        = &_comm->clientUser;
                    rei.uoip        = &_comm->proxyUser;
                }

                rule_engine_context_manager_type re_ctx_mgr(re_plugin_globals->global_re_mgr, &rei);

                // Always run the finally-PEP at scope exit.
                at_scope_exit invoke_finally_pep{[&] {
                    error finally_err = invoke_policy_enforcement_point(re_ctx_mgr,
                                                                        ctx,
                                                                        operation_name,
                                                                        "finally",
                                                                        forward<types_t>(_t)...);

                    if (!finally_err.ok()) {
                        irods::log(PASS(finally_err));
                    }
                }};

                error op_err = SUCCESS();

                // invoke the pre-pep for this operation
                error pre_err = invoke_policy_enforcement_point(
                                    re_ctx_mgr,
                                    ctx,
                                    operation_name,
                                    "pre",
                                    std::forward<types_t>(_t)...);

                if (pre_err.code() != RULE_ENGINE_SKIP_OPERATION) {
                    if (!pre_err.ok()) {
                        // if the pre-pep fails, invoke the exception pep
                        error except_err = invoke_policy_enforcement_point(
                                           re_ctx_mgr,
                                           ctx,
                                           operation_name,
                                           "except",
                                           std::forward<types_t>(_t)...);

                        if (!except_err.ok()) {
                            irods::log(PASS(except_err));
                        }

                        return pre_err.code();
                    }

                    using adapted_func_type = std::function<error(irods::plugin_context&, rsComm_t*, types_t...)>;
                    adapted_func_type adapted_fcn{api_call_adaptor<types_t...>(fcn)};
                    op_err = adapted_fcn(ctx, _comm, forward<types_t>(_t)...);

                    if (!op_err.ok() && !is_acceptable_error(op_err.code())) {
                        // if the operation fails, invoke the exception pep
                        error except_err = invoke_policy_enforcement_point(
                                               re_ctx_mgr,
                                               ctx,
                                               operation_name,
                                               "except",
                                               forward<types_t>(_t)...);

                        if (!except_err.ok()) {
                            irods::log(PASS(except_err));
                        }

                        return op_err.code();
                    }
                } // error_code != RULE_ENGINE_SKIP_OPERATION

                // invoke the post-pep for this operation
                error post_err = invoke_policy_enforcement_point(
                                     re_ctx_mgr,
                                     ctx,
                                     operation_name,
                                     "post",
                                     forward<types_t>(_t)...);

                if (!post_err.ok()) {
                    // if the post-pep fails, invoke the exception pep
                    error except_err = invoke_policy_enforcement_point(
                                           re_ctx_mgr,
                                           ctx,
                                           operation_name,
                                           "except",
                                           forward<types_t>(_t)...);

                    if (!except_err.ok()) {
                        irods::log(PASS(except_err));
                    }

                    return post_err.code();
                }

                return op_err.code();
#else // ENABLE_RE
                return fcn(_comm, _t...);
#endif // ENABLE_RE
            }
            catch (const boost::bad_any_cast&) {
                std::string msg( "failed for call - " );
                msg += operation_name;
                irods::log(ERROR(INVALID_ANY_CAST, msg));
                return INVALID_ANY_CAST;
            }

            return 0;
        } // call_handler

        template <typename ...Args>
        int call_handler_without_policy(rsComm_t* _comm, Args... _args)
        {
            if (!operations_.has_entry(operation_name)) {
                rodsLog(LOG_ERROR, "missing api operation [%s]", operation_name.c_str());
                return SYS_INVALID_INPUT_PARAM;
            }

            try {
                using fcn_t = std::function<int(rsComm_t*, Args...)>;
                fcn_t fcn = boost::any_cast<fcn_t>(operations_[operation_name]);
                return fcn(_comm, _args...);
            }
            catch (const boost::bad_any_cast&) {
                std::string msg = "failed for call - ";
                msg += operation_name;
                irods::log(ERROR(INVALID_ANY_CAST, msg));
                return INVALID_ANY_CAST;
            }

            return 0;
        } // call_handler_without_policy

        // ctors
        api_entry( apidef_t& );

        api_entry( const api_entry& );

        // operators
        api_entry& operator=( const api_entry& );

        // attributes
        int            apiNumber;      /* the API number */
        char*          apiVersion;     /* The API version of this call */
        int            clientUserAuth; /* Client user authentication level.
                                    * NO_USER_AUTH, REMOTE_USER_AUTH,
                                    * LOCAL_USER_AUTH, REMOTE_PRIV_USER_AUTH or
                                    * LOCAL_PRIV_USER_AUTH */
        int            proxyUserAuth;                    /* same for proxyUser */
        const char*    inPackInstruct; /* the packing instruct for the input
                                    * struct */
        int inBsFlag;                  /* input bytes stream flag. 0 ==> no input
                                    * byte stream. 1 ==> we have an input byte
                                    * stream */
        const char*    outPackInstruct;/* the packing instruction for the
                                    * output struct */
        int            outBsFlag;      /* output bytes stream. 0 ==> no output byte
                                    * stream. 1 ==> we have an output byte stream
                                    */
        funcPtr        call_wrapper; // wraps the api call for type casting
        std::string    in_pack_key;
        std::string    out_pack_key;
        std::string    in_pack_value;
        std::string    out_pack_value;
        std::string    operation_name;

        lookup_table< std::string>   extra_pack_struct;

        std::function<void(void*)> clearInStruct;   // free input struct function
        std::function<void(void*)> clearOutStruct;  // free output struct function

    private:
#ifdef ENABLE_RE
        template<typename... types_t>
        error invoke_policy_enforcement_point(
            rule_engine_context_manager_type _re_ctx_mgr,
            plugin_context&                  _ctx,
            const std::string&               _operation_name,
            const std::string&               _class,
            types_t...                       _t)
        {
            using log = irods::experimental::log::rule_engine;

            bool ret = false;
            error saved_op_err = SUCCESS();
            error skip_op_err = SUCCESS();

            for (auto& ns : NamespacesHelper::Instance()->getNamespaces()) {
                std::string rule_name = ns + "pep_" + _operation_name + "_" + _class;

                if (RuleExistsHelper::Instance()->checkOperation( rule_name ) ) {
                    if (_re_ctx_mgr.rule_exists(rule_name, ret).ok() && ret) {
                        error op_err = _re_ctx_mgr.exec_rule(rule_name, instance_name_, _ctx, std::forward<types_t>(_t)...);

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
#endif // ENABLE_RE
    }; // class api_entry

    typedef boost::shared_ptr< api_entry > api_entry_ptr;

    /// =-=-=-=-=-=-=-
    /// @brief class which will hold statically compiled and dynamically loaded api handles
    class api_entry_table
        : public lookup_table<api_entry_ptr, size_t, boost::hash<size_t>>
    {
    public:
        api_entry_table(apidef_t defs[], size_t size);

        auto is_plugin_loaded(std::string_view plugin_name) -> bool;
        auto mark_plugin_as_loaded(std::string_view plugin_name) -> void;

    private:
        std::vector<std::string> loaded_plugins_;
    }; // class api_entry_table

    /// =-=-=-=-=-=-=-
    /// @brief load api plugins
    error init_api_table(
        api_entry_table&  _api_tbl,    // table holding api entries
        pack_entry_table& _pack_tbl,   // table for pack struct ref
        bool              _cli_flg = true ); // default to client
} // namespace irods

#endif // IRODS_API_HANDLER_HPP
