/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rodsAgent.c - The main code for rodsAgent
 */

#include <syslog.h>
#include "rodsAgent.h"
#include "reconstants.h"
#include "rsApiHandler.h"
#include "icatHighLevelRoutines.h"
#include "miscServerFunct.h"
#ifdef windows_platform
#include "rsLog.h"
static void NtAgentSetEnvsFromArgs(int ac, char **av);
#endif

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_dynamic_cast.h"
#include "eirods_signal.h"
#include "eirods_client_server_negotiation.h"
#include "eirods_network_factory.h"

/* #define SERVER_DEBUG 1   */
int
main(int argc, char *argv[])
{
    int status;
    rsComm_t rsComm;
    char *tmpStr;

    ProcessType = AGENT_PT;
//sleep(30);
#ifdef RUN_SERVER_AS_ROOT
#ifndef windows_platform
    if (initServiceUser() < 0) {
        exit (1);
    }
#endif
#endif

#ifdef windows_platform
    iRODSNtAgentInit(argc, argv);
#endif

#ifndef windows_platform
    signal(SIGINT, signalExit);
    signal(SIGHUP, signalExit);
    signal(SIGTERM, signalExit);
    /* set to SIG_DFL as recommended by andy.salnikov so that system()
     * call returns real values instead of 1 */
    signal(SIGCHLD, SIG_DFL);
    signal(SIGUSR1, signalExit);
    signal(SIGPIPE, rsPipSigalHandler);

    // register eirods signal handlers
    register_handlers();
#endif

#ifndef windows_platform
#ifdef SERVER_DEBUG
    if (isPath ("/tmp/rodsdebug"))
        sleep (20);
#endif
#endif

#ifdef SYS_TIMING
    rodsLogLevel(LOG_NOTICE);
    printSysTiming ("irodsAgent", "exec", 1);
#endif

    memset (&rsComm, 0, sizeof (rsComm));

    status = initRsCommWithStartupPack (&rsComm, NULL);
 
    // =-=-=-=-=-=-=-
    // manufacture a network object for comms
    eirods::network_object_ptr net_obj;
    eirods::error ret = eirods::network_factory( &rsComm, net_obj );
    if( !ret.ok() ) {
        eirods::log( PASS( ret ) );
    }

    if (status < 0) {
        sendVersion ( net_obj, status, 0, NULL, 0);
        unregister_handlers();
        cleanupAndExit (status);
    }

    /* Handle option to log sql commands */
    tmpStr = getenv (SP_LOG_SQL);
    if (tmpStr != NULL) {
#ifdef IRODS_SYSLOG
        int j = atoi(tmpStr);
        rodsLogSqlReq(j);
#else
        rodsLogSqlReq(1);
#endif
    }

    /* Set the logging level */
    tmpStr = getenv (SP_LOG_LEVEL);
    if (tmpStr != NULL) {
        int i;
        i = atoi(tmpStr);
        rodsLogLevel(i);
    } else {
        rodsLogLevel(LOG_NOTICE); /* default */
    }

#ifdef IRODS_SYSLOG
/* Open a connection to syslog */
    openlog("rodsAgent",LOG_ODELAY|LOG_PID,LOG_DAEMON);
#endif

    status = getRodsEnv (&rsComm.myEnv);

    if (status < 0) {
        sendVersion ( net_obj, SYS_AGENT_INIT_ERR, 0, NULL, 0);
        unregister_handlers();
        cleanupAndExit (status);
    }

#if RODS_CAT
    if (strstr(rsComm.myEnv.rodsDebug, "CAT") != NULL) {
        chlDebug(rsComm.myEnv.rodsDebug);
    }
#endif

    status = initAgent (RULE_ENGINE_TRY_CACHE, &rsComm);

#ifdef SYS_TIMING
    printSysTiming ("irodsAgent", "initAgent", 0);
#endif

    if (status < 0) {
        sendVersion ( net_obj, SYS_AGENT_INIT_ERR, 0, NULL, 0);
        unregister_handlers();
        cleanupAndExit (status);
    }

    /* move configConnectControl behind initAgent for now. need zoneName if
     * the user does not specify one in the input */
    initConnectControl ();

    if (rsComm.clientUser.userName[0] != '\0') {
        status = chkAllowedUser (rsComm.clientUser.userName,
                                 rsComm.clientUser.rodsZone);

        if (status < 0) {
            sendVersion ( net_obj, status, 0, NULL, 0);
            unregister_handlers();
            cleanupAndExit (status);
        }
    }

    // =-=-=-=-=-=-=-
    // handle negotiations with the client regarding TLS if requested
    std::string neg_results;
    ret = eirods::client_server_negotiation_for_server( net_obj, neg_results );
    if( !ret.ok() || neg_results == eirods::CS_NEG_FAILURE ) {
        eirods::log( PASS( ret ) );
        // =-=-=-=-=-=-=-
        // send a 'we failed to negotiate' message here??
        // or use the error stack rule engine thingie
        sendVersion( net_obj, SYS_AGENT_INIT_ERR, 0, NULL, 0 );
        unregister_handlers();
        cleanupAndExit( ret.code() );
    
    } else {
        // =-=-=-=-=-=-=-
        // copy negotiation results to comm for action by network objects
        strncpy( rsComm.negotiation_results, neg_results.c_str(), MAX_NAME_LEN );
        //rsComm.ssl_do_accept = 1;
    
    }

    /* send the server version and atatus as part of the protocol. Put
     * rsComm.reconnPort as the status */
    ret = sendVersion( net_obj, status, rsComm.reconnPort,
                          rsComm.reconnAddr, rsComm.cookie);

    if( !ret.ok() ) {
        sendVersion (net_obj, SYS_AGENT_INIT_ERR, 0, NULL, 0);
        unregister_handlers();
        cleanupAndExit (status);
    }

#ifdef SYS_TIMING
    printSysTiming ("irodsAgent", "sendVersion", 0);
#endif

    logAgentProc (&rsComm);
    
    bool done = true;
    while(!done) {
        sleep(1);
    }
#if 1 
    // =-=-=-=-=-=-=-
    // call initialization for network plugin as negotiated 
    eirods::network_object_ptr new_net_obj;
    ret = eirods::network_factory( &rsComm, new_net_obj );
    if( !ret.ok() ) {
        return ret.code();
    }

    ret = sockAgentStart( new_net_obj );
    if( !ret.ok() ) {
        eirods::log( PASS( ret ) );
        return ret.code();
    }

    new_net_obj->to_server( &rsComm );
#endif 
    status = agentMain( &rsComm );

#if 1 
    // =-=-=-=-=-=-=-
    // call initialization for network plugin as negotiated 
    ret = sockAgentStop( new_net_obj );
    if( !ret.ok() ) {
        eirods::log( PASS( ret ) );
        return ret.code();
    }

    new_net_obj->to_server( &rsComm );
#endif
    unregister_handlers();
    cleanupAndExit (status);

    return (status);
}

int 
agentMain (rsComm_t *rsComm)
{
    int status = 0;

    // =-=-=-=-=-=-=-
    // compiler backwards compatibility hack
    // see header file for more details
    eirods::dynamic_cast_hack();

    while (1) {

        if (rsComm->gsiRequest==1) {
            status = igsiServersideAuth(rsComm) ;
            rsComm->gsiRequest=0; 
        }
        if (rsComm->gsiRequest==2) {
            status = ikrbServersideAuth(rsComm) ;
            rsComm->gsiRequest=0; 
        }

        if (rsComm->ssl_do_accept) {
            status = sslAccept(rsComm);
            rsComm->ssl_do_accept = 0;
        }
        if (rsComm->ssl_do_shutdown) {
            status = sslShutdown(rsComm);
            rsComm->ssl_do_shutdown = 0;
        }

        status = readAndProcClientMsg (rsComm, READ_HEADER_TIMEOUT);

        if (status >= 0) {
            continue;
        } else {
            if (status == DISCONN_STATUS) {
                status = 0;
                break;
            } else {
                break;
            }
        }
    }

    // =-=-=-=-=-=-=-
    // determine if we even need to connect, break the
    // infinite reconnect loop.
    if( !resc_mgr.need_maintenance_operations() ) {
        return status;
    }

    // =-=-=-=-=-=-=-
    // find the icat host
    rodsServerHost_t *rodsServerHost = 0;
    status = getRcatHost( MASTER_RCAT, 0, &rodsServerHost );
    if( status < 0 ) {
        eirods::log( ERROR( status, "getRcatHost failed." ) );
        return status;
    }
    
    // =-=-=-=-=-=-=-
    // connect to the icat host
    status = svrToSvrConnect ( rsComm, rodsServerHost );
    if( status < 0 ) {
        eirods::log( ERROR( status, "svrToSvrConnect failed." ) );
        return status;
    }

    // =-=-=-=-=-=-=-
    // call post disconnect maintenance operations before exit
    status = resc_mgr.call_maintenance_operations( rodsServerHost->conn );

    return (status);
}





























