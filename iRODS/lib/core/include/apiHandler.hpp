/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* apiHandler.h - header file for apiHandler.h
 */



#ifndef API_HANDLER_HPP
#define API_HANDLER_HPP

// =-=-=-=-=-=-=-
// boost includes
#include "rods.h"
#include "packStruct.h"
#include "irods_lookup_table.hpp"
#include "irods_plugin_base.hpp"
#include "irods_stacktrace.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/any.hpp"

#include <functional>

namespace irods {

// NOOP function for clearInStruct
    void clearInStruct_noop( void* );

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
        packInstruct_t inPackInstruct; /* the packing instruct for the input
                                        * struct */
        int inBsFlag;                  /* input bytes stream flag. 0 ==> no input
                                        * byte stream. 1 ==> we have an input byte
                                        * stream */
        packInstruct_t outPackInstruct;/* the packing instruction for the
                                        * output struct */
        int            outBsFlag;      /* output bytes stream. 0 ==> no output byte
                                        * stream. 1 ==> we have an output byte stream
                                        */
        boost::any     svrHandler;     /* the server handler. should be defined NULL for
                                        * client */
		
        const char *   operation_name;

        std::function<void( void* )> clearInStruct;	// free input struct function

        int(*call_wrapper)(...);        // wraps the api call for type casting 

    }; // struct apidef_t

    template < typename... types_t >
    class api_call_adaptor {
        std::function<int(types_t...)> fcn_;
    public:
        api_call_adaptor( std::function<int(types_t...)> _fcn ): fcn_(_fcn) {
        }

        irods::error operator()( irods::plugin_context&, types_t... _t ) {
            int ret = fcn_( _t... );
            if( ret >= 0 ) {
                return CODE( ret );
            }
            else {
                return ERROR( ret, "fail" );
            }
        }

    }; // class api_call_adaptor


    class api_entry : public irods::plugin_base {
        public:
            /// =-=-=-=-=-=-=-
            /// @brief adaptor from old api sig to new plugin sig
            template<typename... types_t>
                error add_operation(
                        const std::string& _op,
                        std::function<int(types_t...)> _f ) {
                    if ( _op.empty() ) {
                        std::stringstream msg;
                        msg << "empty operation key [" << _op << "]";
                        return ERROR(
                                SYS_INVALID_INPUT_PARAM,
                                msg.str() );
                    }

                    operation_name = _op;
                    operations_[operation_name] = _f;

                    return SUCCESS();

                } // add_operation

            template<typename... types_t>
                int call_handler(
                    types_t... _t ) {
                    using namespace std;
                    if( !operations_.has_entry(operation_name) ) {
                        rodsLog(
                            LOG_ERROR,
                            "missing api operation [%s]",
                            operation_name.c_str() );
                        return SYS_INVALID_INPUT_PARAM;
                    }

                    try {
                        typedef std::function<int(types_t...)> fcn_t;
                        fcn_t fcn = boost::any_cast<fcn_t>( operations_[ operation_name ] );
                        #ifdef ENABLE_RE
                        ruleExecInfo_t rei;
                        memset( ( char* )&rei, 0, sizeof( ruleExecInfo_t ) );
                        dynamic_operation_execution_manager<
                            default_re_ctx,
                            default_ms_ctx,
                            DONT_AUDIT_RULE > rex_mgr(
                                shared_ptr<
                                    rule_engine_context_manager<
                                        default_re_ctx,
                                        default_ms_ctx,
                                        DONT_AUDIT_RULE> >(
                                            new rule_engine_context_manager<
                                                default_re_ctx,
                                                default_ms_ctx,
                                                DONT_AUDIT_RULE >(
                                                    re_plugin_globals->global_re_mgr, &rei)));

                        std::function<error(irods::plugin_context&,types_t...)> adapted_fcn(
                                ( api_call_adaptor<types_t...>(fcn) ) );

                        irods::plugin_property_map prop_map;
                        irods::plugin_context ctx(NULL,prop_map);
                        irods::error op_err = rex_mgr.call(
                                                  "api_instance",
                                                  operation_name,
                                                  adapted_fcn,
                                                  ctx,
                                                  _t...);
                        return op_err.code();
                        #else
                        return fcn(_t...);
                        #endif
                    }
                    catch( const boost::bad_any_cast& ) {
                        std::string msg( "failed for call - " );
                        msg += operation_name;
                        irods::log( ERROR(
                                    INVALID_ANY_CAST,
                                    msg ) );
                        return INVALID_ANY_CAST;
                    }

                    return 0;

                } // call_handler


            // =-=-=-=-=-=-=-
            // ctors
            api_entry(
                apidef_t& );

            api_entry( const api_entry& );

            // =-=-=-=-=-=-=-
            // operators
            api_entry& operator=( const api_entry& );

            // =-=-=-=-=-=-=-
            // attributes
            int            apiNumber;      /* the API number */
            char*          apiVersion;     /* The API version of this call */
            int            clientUserAuth; /* Client user authentication level.
                                        * NO_USER_AUTH, REMOTE_USER_AUTH,
                                        * LOCAL_USER_AUTH, REMOTE_PRIV_USER_AUTH or
                                        * LOCAL_PRIV_USER_AUTH */
            int            proxyUserAuth;                    /* same for proxyUser */
            packInstruct_t inPackInstruct; /* the packing instruct for the input
                                        * struct */
            int inBsFlag;                  /* input bytes stream flag. 0 ==> no input
                                        * byte stream. 1 ==> we have an input byte
                                        * stream */
            packInstruct_t outPackInstruct;/* the packing instruction for the
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

            std::function<void( void* )> clearInStruct;		//free input struct function

    }; // class api_entry

    typedef boost::shared_ptr< api_entry > api_entry_ptr;


/// =-=-=-=-=-=-=-
/// @brief class which will hold statically compiled and dynamically loaded api handles
    class api_entry_table : public lookup_table< api_entry_ptr, size_t, boost::hash< size_t > > {
        public:
            api_entry_table( apidef_t[], size_t );
            ~api_entry_table();

    }; // class api_entry_table


/// =-=-=-=-=-=-=-
/// @brief class to hold packing instruction and free function
    class pack_entry {
        public:
            std::string packInstruct;

            // ctor
            pack_entry() {};
            pack_entry( const pack_entry& );


            // dtor
            ~pack_entry() {};

            // assignment operator
            pack_entry& operator=( const pack_entry& );
    };


/// =-=-=-=-=-=-=-
/// @brief class which will hold the map of pack struct entries
    class pack_entry_table : public lookup_table< pack_entry > {
        public:
            pack_entry_table( packInstructArray_t[] );
            ~pack_entry_table();

    }; // class api_entry_table


/// =-=-=-=-=-=-=-
/// @brief load api plugins
    error init_api_table(
        api_entry_table&  _api_tbl,    // table holding api entries
        pack_entry_table& _pack_tbl,   // table for pack struct ref
        bool              _cli_flg = true ); // default to client
}; // namespace irods

#endif          /* API_HANDLER_H */
