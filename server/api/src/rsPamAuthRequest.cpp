#include "irods/rsPamAuthRequest.hpp"

#include "irods/authenticate.h"
#include "irods/genQuery.h"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/irods_default_paths.hpp"
#include "irods/irods_log.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/pamAuthRequest.h"
#include "irods/sslSockComm.h"
#include "irods/irods_logger.hpp"

#include <sys/wait.h>

int
rsPamAuthRequest( rsComm_t *rsComm, pamAuthRequestInp_t *pamAuthRequestInp,
                  pamAuthRequestOut_t **pamAuthRequestOut ) {
    rodsServerHost_t *rodsServerHost;
    int status;

    status = getAndConnRcatHost(rsComm, PRIMARY_RCAT, (const char*) rsComm->clientUser.rodsZone, &rodsServerHost);
    if ( status < 0 ) {
        return status;
    }
    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
        std::string svc_role;
        irods::error ret = get_catalog_service_role(svc_role);
        if(!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }

        if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            status = _rsPamAuthRequest(
                         rsComm,
                         pamAuthRequestInp,
                         pamAuthRequestOut );
        } else if( irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
            status = SYS_NO_RCAT_SERVER_ERR;
        } else {
            rodsLog(
                LOG_ERROR,
                "role not supported [%s]",
                svc_role.c_str() );
            status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
        }
    }
    else {
        /* protect the PAM plain text password by
           using an SSL connection to the remote ICAT */
        status = sslStart( rodsServerHost->conn );
        if ( status ) {
            rodsLog( LOG_NOTICE, "rsPamAuthRequest: could not establish SSL connection, status %d",
                     status );
            return status;
        }

        status = rcPamAuthRequest( rodsServerHost->conn, pamAuthRequestInp,
                                   pamAuthRequestOut );
        sslEnd( rodsServerHost->conn );
        rcDisconnect( rodsServerHost->conn );
        rodsServerHost->conn = NULL;
        if ( status < 0 ) {
            rodsLog( LOG_NOTICE, "rsPamAuthRequest: rcPamAuthRequest to remote server failed, status %d",
                     status );
        }
    }
    return status;
}

/*
 Fork and exec the irodsPamAuthCheck program, which is setuid root, to do
 do the PAM authentication.  The username is on the command line but
 the password is sent on a pipe to be more secure.
 */
int
runPamAuthCheck( char *username, char *password ) {
    int p2cp[2]; /* parent to child pipe */
    int pid, i;
    int status;

    if ( pipe( p2cp ) < 0 ) {
        return SYS_PIPE_ERROR;
    }
    pid = fork();
    if ( pid == -1 ) {
        irods::experimental::log::authentication::error("{}: fork() error: errno=[{}], username=[{}].", __func__, errno, username ? username : "nullptr");
        return SYS_FORK_ERROR;
    }

    if ( pid )  {
        /*
          This is still the parent.  Write the message to the child and
          then wait for the exit and status.
        */
        if ( write( p2cp[1], password, strlen( password ) ) == -1 ) {
            int errsv = errno;
            irods::log( ERROR( errsv, "Error during write from parent to child." ) );
        }
        close( p2cp[1] );
        waitpid( pid, &status, 0 );
        return status;
    }
    else {
        /* This is the child */
        close( 0 );        /* close current stdin */
        if ( dup2( p2cp[0], 0 ) == -1 ) { /* Make stdin come from read end of the pipe */
            int errsv = errno;
            irods::log( ERROR( errsv, "Error duplicating the file descriptor." ) );
        }
        close( p2cp[1] );
        const auto binary = irods::get_irods_sbin_directory() / "irodsPamAuthCheck";
        i = execl(binary.c_str(), binary.c_str(), username, static_cast<char*>(nullptr)); // NOLINT(cppcoreguidelines-pro-type-vararg)
        perror( "execl" );
        printf( "execl failed %d\n", i );
    }
    irods::experimental::log::api::error("{}: Returning default value [SYS_FORK_ERROR]", __func__);
    return SYS_FORK_ERROR; /* avoid compiler warning */
}

int
_rsPamAuthRequest( rsComm_t *rsComm, pamAuthRequestInp_t *pamAuthRequestInp,
                   pamAuthRequestOut_t **pamAuthRequestOut ) {
    int status = 0;

    *pamAuthRequestOut = ( pamAuthRequestOut_t * )
                         malloc( sizeof( pamAuthRequestOut_t ) );
    memset( ( char * )*pamAuthRequestOut, 0, sizeof( pamAuthRequestOut_t ) );

    /* Normal mode, fork/exec setuid program to do the Pam check */
    status = runPamAuthCheck( pamAuthRequestInp->pamUser,
                              pamAuthRequestInp->pamPassword );
    if ( status == 256 ) {
        status = PAM_AUTH_PASSWORD_FAILED;
    }
    else {
        /* the exec failed or something (irodsPamAuthCheck not built perhaps) */
        if ( status != 0 ) {
            status = PAM_AUTH_NOT_BUILT_INTO_SERVER;
        }
    }

    if ( status ) {
        return status;
    }

    pamAuthRequestOut_t* result = *pamAuthRequestOut;

    // Plus 1 for null terminator.
    constexpr std::size_t password_buffer_size = MAX_PASSWORD_LEN + 1;
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
    result->irodsPamPassword = static_cast<char*>(std::malloc(password_buffer_size));
    if (nullptr == result->irodsPamPassword) {
        return SYS_MALLOC_ERR;
    }

    std::memset(result->irodsPamPassword, 0, password_buffer_size);
    status = chlUpdateIrodsPamPassword(rsComm,
                                       pamAuthRequestInp->pamUser,
                                       pamAuthRequestInp->timeToLive,
                                       nullptr,
                                       &result->irodsPamPassword,
                                       password_buffer_size);
    return status;
}
