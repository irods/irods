/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* apiHandler.h - header file for apiHandler.h
 */



#ifndef API_HANDLER_HPP
#define API_HANDLER_HPP

// =-=-=-=-=-=-=-
// boost includes
#include "rods.h"
#include "packStruct.hpp"
#include "irods_lookup_table.hpp"
#include "irods_plugin_base.hpp"
#include "boost/shared_ptr.hpp"

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
        funcPtr        svrHandler;     /* the server handler. should be defined NULL for
                                        * client */

        void ( *clearInStruct )( void* ); // free input struct function

    }; // struct apidef_t

    class api_entry : public irods::plugin_base {
        public:
            // =-=-=-=-=-=-=-
            // ctors
            api_entry(
                apidef_t& );

            api_entry( const api_entry& );

            // =-=-=-=-=-=-=-
            // operators
            api_entry& operator=( const api_entry& );

            // =-=-=-=-=-=-=-
            // lazy loader for plugin operations
            irods::error delay_load( void* _h );

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
            funcPtr        svrHandler;     /* the server handler. should be defined NULL for
                                        * client */
            std::string    in_pack_key;
            std::string    out_pack_key;
            std::string    in_pack_value;
            std::string    out_pack_value;
            std::string    fcn_name_;

            lookup_table< std::string>   extra_pack_struct;

            boost::function<void( void* )> clearInStruct;		//free input struct function

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
