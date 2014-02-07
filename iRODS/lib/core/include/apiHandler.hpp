/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* apiHandler.h - header file for apiHandler.h
 */



#ifndef API_HANDLER_HPP
#define API_HANDLER_HPP

#include "rods.hpp"
#include "sockComm.hpp"
#include "packStruct.hpp"
#include "irods_lookup_table.hpp"
#include "irods_plugin_base.hpp"

namespace irods {
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

    }; // class api_entry

    /// =-=-=-=-=-=-=-
    /// @brief class which will hold statically compiled and dynamically loaded api handles
    class api_entry_table : public lookup_table< api_entry*, size_t, boost::hash< size_t > > {
    public:
        api_entry_table( apidef_t[], size_t );
        ~api_entry_table();

    }; // class api_entry_table

};

#endif          /* API_HANDLER_H */
