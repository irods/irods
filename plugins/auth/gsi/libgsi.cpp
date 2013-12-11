/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "irods_error.h"
#include "irods_gsi_object.h"
#include "irods_auth_plugin.h"
#include "irods_auth_constants.h"

#include <gssapi.h>

#include <string>

extern "C" {
    // =-=-=-=-=-=-=-
    // 1. Define plugin Version Variable, used in plugin
    //    creation when the factory function is called.
    //    -- currently only 1.0 is supported.
    double IRODS_PLUGIN_INTERFACE_VERSION=1.0;

    // Define some useful globals
    static const unsigned int SCRATCH_BUFFER_SIZE 20000
    static char igsiScratchBuffer[SCRATCH_BUFFER_SIZE];
    static int igsiTokenHeaderMode = 1;  /* 1 is the normal mode, 
                                            0 means running in a non-token-header mode, ie Java; dynamically cleared. */

    void gsi_log_error_1(
        rError_t* _r_error,
        const char *callerMsg,
        OM_uint32 code,
        int type,
        bool is_client)
    {
        OM_uint32 majorStatus, minorStatus;
        gss_buffer_desc msg;
        unsigned int msg_ctx;
        int status;
        std::string whichSide;

        if (is_client) {
            whichSide = "Client side:";
        }
        else {
            whichSide = "On iRODS-Server side:";
        }

        msg_ctx = 0;
        status = GSI_ERROR_FROM_GSI_LIBRARY;
        do {
            majorStatus = gss_display_status(&minorStatus, code,
                                             type, GSS_C_NULL_OID,
                                             &msg_ctx, &msg);
            rodsLogAndErrorMsg( LOG_ERROR, _r_error, status,
                                "%sGSS-API error %s: %s", whichSide.c_str(), callerMsg,
                                (char *) msg.value);
            (void) gss_release_buffer(&minorStatus, &msg);
        } while(msg_ctx);
    }

    void gsi_log_error(
        rError_t* _r_error,
        const char *msg,
        OM_uint32 majorStatus,
        OM_uint32 minorStatus,
        bool is_client)
    {
        gsi_log_error_1(_r_error, msg, majorStatus, GSS_C_GSS_CODE, is_client);
        gsi_log_error_1(_r_error, msg, minorStatus, GSS_C_MECH_CODE, is_client);
    }

    irods::error gsi_no_op(
        irods::gsi_object_ptr _go)
    {
        irods::error result = SUCCESS();
        return result;
    }
    
    irods::error gsi_setup_creds(
        irods::gsi_object_ptr _go,
        bool is_client,
        std::string& _rtn_name)
    {
        irods::error result = SUCCESS();
        OM_uint32 major_status;
        OM_uint32 minor_status;
        gss_name_t my_name = GSS_C_NO_NAME;
        gss_name_t my_name_2 = GSS_C_NO_NAME;
        gss_cred_id_t tmp_creds;
        gss_buffer_desc client_name2;
        gss_OID doid2;

        if(_go->creds() == GSS_C_NO_CREDENTIAL) {
            major_status = gss_acquire_cred(&minor_status, my_name, 0, GSS_C_NULL_OID_SET, GSS_C_ACCEPT, &tmp_creds, NULL, NULL);
        }
        else {
            major_status = GSS_S_COMPLETE;
        }

        if(major_status != GSS_S_COMPLETE) {
            gsi_log_error(_go->r_error(), "acquiring credentials", major_status, minor_status, is_client);
            return ERROR(GSI_ERROR_ACQUIRING_CREDS, "Failed acquiring credentials.");
        }

        gss_release_name(&minor_status, &my_name);

        major_status = gss_inquire_cred(&minor_status, tmp_creds, &my_name_2, 
                                        NULL, NULL, NULL);
        if (major_status != GSS_S_COMPLETE) {
            gsi_log_error(_go->r_error(), "inquire_cred", major_status, minor_status, is_client);
            return ERROR(GSI_ERROR_ACQUIRING_CREDS, "Failed inquiring about credentials.");
        }

        major_status = gss_display_name(&minor_status, my_name_2, &client_name2, &doid2);
        if (major_status != GSS_S_COMPLETE) {
            gsi_log_error(_go->r_error(), "displaying name", major_status, minor_status, is_client);
            return ERROR(GSI_ERROR_DISPLAYING_NAME, "Failed displaying name.");
        }

        if (client_name2.value != 0 && client_name2.length > 0) {
            _rtn_name = std::string((char *)client_name2.value, client_name2.length);
        }

        /* release the name structure */
        major_status = gss_release_name(&minor_status, &my_name_2);
        if (major_status != GSS_S_COMPLETE) {
            gsi_log_error(_go->r_error(), "releasing name", major_status, minor_status, is_client);
            return ERROR(GSI_ERROR_RELEASING_NAME, "Failed releasing name.");
        }
        
        (void) gss_release_buffer(&minor_status, &client_name2);
        
        return result;
    }

    /// @brief Setup auth object with relevant information
    irods::error gsi_auth_client_start(
        irods::auth_plugin_context& _ctx,
        rcComm_t* _comm,
        const char* _context)
    {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = _ctx.valid<irods::gsi_auth_object>();
        if((result = ASSERT_PASS(ret, "Invalid plugin context")).ok()) {
            if((result = ASSERT_ERROR(_comm != NULL, SYS_INVALID_INPUT_PARAM, "Null rcComm_t pointer.")).ok()) {

                irods::gsi_auth_object_ptr ptr = boost::dynamic_pointer_cast<irods::gsi_auth_object>(_ctx.fco());

                // set the user name from the conn
                ptr->user_name(_comm->proxyUser.userName);

                // set the zone name from the conn
                ptr->zone_name(_comm->proxyUser.rodsZone);

                // set the socket from the conn
                ptr->sock(_comm->sock);
            }
        }
        
        return result;
    }

    /// @brief no op
    irods::error gsi_auth_success_stub(void)
    {
        irods::error result = SUCCESS();
        return result;
    }

    /// @brief Import the specified name into a GSI name
    irods::error gsi_import_name(
        const char* _service_name, // GSI service name
        gss_name_t* _target_name)      // GSI target name
    {
        irods::error result = SUCCESS();
        gss_buffer_desc name_buffer;
        
        *_target_name = GSS_C_NO_NAME;
        if(_service_name != NULL && strlen(_service_name) > 0) {
            name_buffer.value = _service_name;
            name_buffer.length = strlen(_service_name);

            OM_uint32 minor_status;
            OM_uint32 major_status = gss_import_name(&minor_status, &name_buffer, (gss_OID) gss_nt_service_name_gsi, _target_name);

            if(!(result = ASSERT_ERROR(major_status != GSS_S_COMPLETE, GSI_ERROR_IMPORT_NAME, "Failed importing name.")).ok()) {
                /* could use "if (GSS_ERROR(majorStatus))" but I believe it should
                   always be GSS_S_COMPLETE if successful  */
                gsi_log_error ("importing name (igsiEstablishContextClientside)", major_status, minor_status);
            } 
        }
        return result;
    }

    /// @brief Write a GSI buffer.
    /**
      Write a buffer to the network, continuing with subsequent writes if
      the write system call returns with only some of it sent.
    */
    irods::error gsi_write_all(
        int fd,
        char *buf,
        unsigned int nbyte)
    {
        int ret;
        char *ptr;

        for (ptr = buf; nbyte; ptr += ret, nbyte -= ret) {
            ret = write(fd, ptr, nbyte);
            if (ret < 0) {
                if (errno == EINTR)
                    continue;
                return (ret);
            } else if (ret == 0) {
                return (ptr - buf);
            }
        }
        if (igsiDebugFlag > 0)
            fprintf(stderr, "_igsiWriteAll, wrote=%d\n", ptr - buf);
        return (ptr - buf);
    }

    /// @brief Send a GSI token
    /**
      Send a token (which is a buffer and a length); write the token length (as a network long) and then the token data on the file
      descriptor.  It returns 0 on success, and -1 if an error occurs or if it could not write all the data.
    */
    irods::error gsi_send_token(
        gss_buffer_desc* _send_tok,
        int _fd)
    {
        irods::error result = SUCCESS();
        irods::error ret;
        int len;
        char *cp;
        unsigned int bytes_written;
        
        if (igsiTokenHeaderMode) {
            len = htonl(_send_tok->length);

            cp = (char *) &len;
            // Apparent hack to handle len variables of greater than 4 bytes. Should be safe since token lengths should likely never
            // be greater than 4 billion but adding a check here to be sure
            if (sizeof(len) > 4) {
                if((result = ASSERT_ERROR(((len << ((sizeof(len) - 4) * 8)) >> ((sizeof(len) - 4) * 8)) == len, GSI_ERROR_SENDING_TOKEN_LENGTH,
                                          "Token length has significant bits past 4 bytes.")).ok()) {
                    cp += sizeof(len) - 4;
                }
            }
            if(result.ok()) {
                ret = _igsiWriteAll(fd, cp, 4, &bytes_written);
                if ((result = ASSERT_PASS(ret, "Error sending GSI token length.")).ok()) {
                    if (!(result = ASSERT_ERROR(bytes_written == 4, GSI_ERROR_SENDING_TOKEN_LENGTH, "Error sending token data: %u of %u bytes written.",
                                                bytes_written, _send_tok->length)).ok()) {
                        rodsLogAndErrorMsg( LOG_ERROR, igsi_rErrorPtr, status,
                                            "sending token data: %d of %d bytes written",
                                            bytes_written, _send_tok->length);
                    }
                }
            }
        }

        if(result.ok()) {
            ret = _igsiWriteAll(fd, (char *)_send_tok->value, _send_tok->length, &bytes_written);
            if ((result = ASSERT_PASS(ret, "Error sending token data2.")).ok()) {
                
                if (!(result = ASSERT_ERROR(bytes_written == _send_tok->length, GSI_ERROR_SENDING_TOKEN_LENGTH,
                                            "Sending token data2: %u of %u bytes written.", bytes_written, _send_tok->length)).ok()) {
                    rodsLogAndErrorMsg( LOG_ERROR, igsi_rErrorPtr, status,
                                        "sending token data2: %u of %u bytes written",
                                        bytes_written, _send_tok->length);
                }
            }
        }
        
        return result;
    }
    
    /// @brief Establish context - take the auth request results and massage them for the auth response call
    irods::error gsi_auth_establish_context(
        irods::auth_plugin_context& _ctx,
        authRequestOut_t* _req,
        authResponseInp_t* _resp)
    {
        irods::error result = SUCCESS();
        irods::error ret;

        ret = _ctx.valid<irods::gsi_auth_object>();
        if((result = ASSERT_PASS(ret, "Invalid plugin context.")).ok()) {
            if((result = ASSERT_ERROR(_req != NULL && _resp != NULL, "Request or response pointer is null.")).ok()) {

                irods::gsi_auth_object_ptr ptr = boost::dynamic_pointer_cast<irods::gsi_auth_object>(_ctx.fco());
                
                int fd;
                int status;

                fd = ptr->sock();

                igsi_rErrorPtr = ptr->r_error();

                gss_OID oid = GSS_C_NULL_OID;
                gss_buffer_desc send_tok, recv_tok, *tokenPtr;
                gss_name_t target_name;
                OM_uint32 majorStatus, minorStatus;
                OM_uint32 flags = 0;
                
                // overload the user of the username in the response structure
                char* serviceName = _resp->username;
                ret = gsi_import_name(_resp->username, &target_name);
                if((result = ASSERT_PASS(ret, "Failed to import username into GSI.")).ok()) {
                    
                    /*
                     * Perform the context-establishment loop.
                     *
                     * On each pass through the loop, tokenPtr points to the token
                     * to send to the server (or GSS_C_NO_BUFFER on the first pass).
                     * Every generated token is stored in send_tok which is then
                     * transmitted to the server; every received token is stored in
                     * recv_tok, which tokenPtr is then set to, to be processed by
                     * the next call to gss_init_sec_context.
                     * 
                     * GSS-API guarantees that send_tok's length will be non-zero
                     * if and only if the server is expecting another token from us,
                     * and that gss_init_sec_context returns GSS_S_CONTINUE_NEEDED if
                     * and only if the server has another token to send us.
                     */

                    tokenPtr = GSS_C_NO_BUFFER;
                    context[fd] = GSS_C_NO_CONTEXT;
                    flags = GSS_C_MUTUAL_FLAG | GSS_C_REPLAY_FLAG;
                 
                    do {
                        majorStatus = gss_init_sec_context(&minorStatus,
                                                           myCreds, &context[fd], target_name, oid, 
                                                           flags, 0, 
                                                           NULL,           /* no channel bindings */
                                                           tokenPtr, NULL, /* ignore mech type */
                                                           &send_tok, &context_flags, 
                                                           NULL); /* ignore time_rec */

                        /* since recv_tok is not malloc'ed, don't need to call
                           gss_release_buffer, instead clear it. */
                        memset (igsiScratchBuffer, 0, SCRATCH_BUFFER_SIZE);

                        if(!(result = ASSERT_ERROR(majorStatus != GSS_S_COMPLETE && majorStatus != GSS_S_CONTINUE_NEEDED,
                                                  GSI_ERROR_INIT_SECURITY_CONTEXT, "Failed initializing GSI context.")).ok()) {
                            gsi_log_error("initializing context", majorStatus, minorStatus);
                            (void) gss_release_name(&minorStatus, &target_name);
                        }
                        else {

                            ret = gsi_send_token(&send_tok, fd);
                            if(!(result = ASSERT_PASS(ret, "Failed sending GSI token.")).ok()) {
                                (void) gss_release_buffer(&minorStatus, &send_tok);
                                (void) gss_release_name(&minorStatus, &target_name);
                            }
                            else {
                                (void) gss_release_buffer(&minorStatus, &send_tok);
                                
                                if (majorStatus == GSS_S_CONTINUE_NEEDED) {
                                    recv_tok.value = &igsiScratchBuffer;
                                    recv_tok.length = SCRATCH_BUFFER_SIZE;
                                    if (_igsiRcvToken(fd, &recv_tok) <= 0) {
                                        (void) gss_release_name(&minorStatus, &target_name);
                                        return GSI_ERROR_RELEASING_NAME;
                                    }
                                    tokenPtr = &recv_tok;
                                }
                            }
                        }
                    } while (result.ok() && majorStatus == GSS_S_CONTINUE_NEEDED);
                        
                    if (serviceName != 0 && strlen(serviceName)>0) {
                        (void) gss_release_name(&minorStatus, &target_name);
                    }
                    
                    if (igsiDebugFlag > 0)
                        _igsiDisplayCtxFlags();
                    
#if defined(IGSI_TIMING)
                    (void) gettimeofday(&endTimeFunc, (struct timezone *) 0);
                    (void) _igsiTime(&sTimeDiff, &endTimeFunc, &startTimeFunc);
                    fSec =
                        (float) sTimeDiff.tv_sec + ((float) sTimeDiff.tv_usec / 1000000.0);
                    
                    /* for IGSI_TIMING  print time */
                    fprintf(stdout, " ---  %.5f sec time taken for executing "
                            "igsiEstablishContextClientside() ---  \n", fSec);
#endif
                }
            }
        }
        return result;
    }

    irods::error gsi_auth_client_request(void)
    {
        irods::error result = SUCCESS();
        return result;
    }

    irods::error gsi_auth_agent_request(void)
    {
        irods::error result = SUCCESS();
        return result;
    }

    irods::error gsi_auth_client_response(void)
    {
        irods::error result = SUCCESS();
        return result;
    }

    irods::error gsi_auth_agent_response(void)
    {
        irods::error result = SUCCESS();
        return result;
    }
    
    /// @brief The gsi auth plugin
    class gsi_auth_plugin : public irods::auth {
    public:
        /// @brief Constructor
        gsi_auth_plugin(
            const std::string& _name, // instance name
            const std::string& _ctx   // context
            ) : irods::auth(_name, _ctx) { }

        /// @brief Destructor
        ~gsi_auth_plugin() { }
        
    }; // class gsi_auth_plugin

    /// @brief factory function to provide an instance of the plugin
    irods::auth* plugin_factory(
        const std::string& _inst_name, // The name of the plugin
        const std::string& _context)   // The context
    {
        irods::auth* result = NULL;
        irods::error ret;
        // create a gsi auth object
        gsi_auth_plugin* gsi = new gsi_auth_plugin(_inst_name, _context);

        if((ret = ASSERT_ERROR(gsi != NULL, SYS_MALLOC_ERR, "Failed to allocate a gsi plugin: \"%s\".",
                               _inst_name.c_str())).ok()) {

            // fill in the operation table mapping call names to function names
            gsi->add_operation( irods::AUTH_SETUP_CREDS,          "gsi_setup_creds");
            gsi->add_operation( irods::AUTH_CLIENT_START,         "gsi_auth_client_start" );
            gsi->add_operation( irods::AUTH_AGENT_START,          "gsi_auth_success_stub" );
            gsi->add_operation( irods::AUTH_ESTABLISH_CONTEXT,    "gsi_auth_establish_context" );
            gsi->add_operation( irods::AUTH_CLIENT_AUTH_REQUEST,  "gsi_auth_client_request" );
            gsi->add_operation( irods::AUTH_AGENT_AUTH_REQUEST,   "gsi_auth_agent_request" );
            gsi->add_operation( irods::AUTH_CLIENT_AUTH_RESPONSE, "gsi_auth_client_response" );
            gsi->add_operation( irods::AUTH_AGENT_AUTH_RESPONSE,  "gsi_auth_agent_response" );
            
            result = dynamic_cast<irods::auth*>(gsi);
            if(!(ret = ASSERT_ERROR(result != NULL, IRODS_INVALID_DYNAMIC_CAST, "Failed to dynamic cast to irods::auth*")).ok()) {
                irods::log(ret);
            }
        }
        else {
            irods::log(ret);
        }
        return result;
    }

};
