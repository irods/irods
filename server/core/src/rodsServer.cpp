/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "rcMisc.h"
#include "rodsServer.hpp"
#include "sharedmemory.hpp"
#include "initServer.hpp"
#include "miscServerFunct.hpp"

#include "irods_logger.hpp"

#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// =-=-=-=-=-=-=-
#include "irods_exception.hpp"
#include "irods_server_state.hpp"
#include "irods_client_server_negotiation.hpp"
#include "irods_network_factory.hpp"
#include "irods_re_plugin.hpp"
#include "irods_server_properties.hpp"
#include "irods_server_control_plane.hpp"
#include "initServer.hpp"
#include "procLog.h"
#include "rsGlobalExtern.hpp"
#include "locks.hpp"
#include "sharedmemory.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/range/iterator_range.hpp>
using namespace boost::filesystem;

#include "sockCommNetworkInterface.hpp"

#include "sys/un.h"

#include "irods_random.hpp"

struct sockaddr_un local_addr{};
int agent_conn_socket{};
bool connected_to_agent{};

pid_t agent_spawning_pid{};
const char socket_dir_template[]{"/tmp/irods_sockets_XXXXXX"};
char agent_factory_socket_dir[sizeof(socket_dir_template)]{};
char agent_factory_socket_file[sizeof(local_addr.sun_path)]{};

uint ServerBootTime;
int SvrSock;

agentProc_t *ConnectedAgentHead = NULL;
agentProc_t *ConnReqHead = NULL;
agentProc_t *SpawnReqHead = NULL;
agentProc_t *BadReqHead = NULL;

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
boost::mutex              ConnectedAgentMutex;
boost::mutex              BadReqMutex;
boost::thread*            ReadWorkerThread[NUM_READ_WORKER_THR];
boost::thread*            SpawnManagerThread;

boost::thread*            PurgeLockFileThread; // JMC - backport 4612

boost::mutex              ReadReqCondMutex;
boost::mutex              SpawnReqCondMutex;
boost::condition_variable ReadReqCond;
boost::condition_variable SpawnReqCond;

std::vector<std::string> setExecArg( const char *commandArgv );

int runIrodsAgentFactory(sockaddr_un agent_addr);

int queueConnectedAgentProc(
    int childPid,
    agentProc_t *connReq,
    agentProc_t **agentProcHead);

namespace {
// We incorporate the cache salt into the rule engine's named_mutex and shared memory object.
// This prevents (most of the time) an orphaned mutex from halting server standup. Issue most often seen
// when a running iRODS installation is uncleanly killed (leaving the file system object used to implement
// boost::named_mutex e.g. in /var/run/shm) and then the iRODS user account is recreated, yielding a different
// UID. The new iRODS user account is then unable to unlock or remove the existing mutex, blocking the server.
    irods::error createAndSetRECacheSalt() {
        // Should only ever set the cache salt once
        try {
            const auto& existing_salt = irods::get_server_property<const std::string>(irods::CFG_RE_CACHE_SALT_KW);
            rodsLog( LOG_ERROR, "createAndSetRECacheSalt: salt already set [%s]", existing_salt.c_str() );
            return ERROR( SYS_ALREADY_INITIALIZED, "createAndSetRECacheSalt: cache salt already set" );
        } catch ( const irods::exception& ) {
            irods::buffer_crypt::array_t buf;
            irods::error ret = irods::buffer_crypt::generate_key( buf, RE_CACHE_SALT_NUM_RANDOM_BYTES );
            if ( !ret.ok() ) {
                rodsLog( LOG_ERROR, "createAndSetRECacheSalt: failed to generate random bytes" );
                return PASS( ret );
            }

            std::string cache_salt_random;
            ret = irods::buffer_crypt::hex_encode( buf, cache_salt_random );
            if ( !ret.ok() ) {
                rodsLog( LOG_ERROR, "createAndSetRECacheSalt: failed to hex encode random bytes" );
                return PASS( ret );
            }

            std::stringstream cache_salt;
            cache_salt << "pid"
                    << static_cast<intmax_t>( getpid() )
                    << "_"
                    << cache_salt_random;

            try {
                irods::set_server_property<std::string>( irods::CFG_RE_CACHE_SALT_KW, cache_salt.str() );
            } catch ( const irods::exception& e ) {
                rodsLog( LOG_ERROR, "createAndSetRECacheSalt: failed to set server_properties" );
                return irods::error(e);
            }

            int ret_int = setenv( SP_RE_CACHE_SALT, cache_salt.str().c_str(), 1 );
            if ( 0 != ret_int ) {
                rodsLog( LOG_ERROR, "createAndSetRECacheSalt: failed to set environment variable" );
                return ERROR( SYS_SETENV_ERR, "createAndSetRECacheSalt: failed to set environment variable" );
            }

            return SUCCESS();
        }
    }

    int get64RandomBytes( char *buf ) {
        const int num_random_bytes = 32;
        const int num_hex_bytes = 2 * num_random_bytes;
        unsigned char random_bytes[num_random_bytes];
        irods::getRandomBytes( random_bytes, sizeof(random_bytes) );

        std::stringstream ss;
        for ( size_t i = 0; i < sizeof(random_bytes); ++i ) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)( random_bytes[i] );
        }

        snprintf( buf, num_hex_bytes + 1, "%s", ss.str().c_str() );
        return 0;
    }
}

static void set_agent_spawner_process_name(const InformationRequiredToSafelyRenameProcess& info) {
    const char* desired_name = "irodsServer: factory";
    const auto l_desired = strlen(desired_name);
    if (l_desired <= info.argv0_size) {
        strncpy(info.argv0, desired_name, info.argv0_size);
    }
}

namespace {

void init_logger()
{
    using log = irods::experimental::log;

    log::init();
    irods::server_properties::instance().capture();
    log::server::set_level(log::get_level_from_config(irods::CFG_LOG_LEVEL_CATEGORY_SERVER_KW));
    log::set_server_type("server");

    if (char hostname[HOST_NAME_MAX]{}; gethostname(hostname, sizeof(hostname)) == 0) {
        log::set_server_host(hostname);
    }
}

} // anonymous namespace

void daemonize()
{
    if (fork()) {
        // End the parent process immediately.
        exit(0);
    }

    if (setsid() < 0) {
        rodsLog(LOG_NOTICE, "serverize: setsid failed, errno = %d\n", errno);
        exit(1);
    }

    close(0);
    close(1);
    close(2);

    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_RDWR);
}

int
main( int argc, char **argv )
{
    init_logger();

    int c;
    int uFlag = 0;
    char tmpStr1[100], tmpStr2[100];

    ProcessType = SERVER_PT;    /* I am a server */

    if (const char* log_level = getenv(SP_LOG_LEVEL); log_level) {
        rodsLogLevel(atoi(log_level));
    }
    else {
        rodsLogLevel(LOG_NOTICE);
    }

    // Issue #3865 - The mere existence of the environment variable sets 
    // the value to 1.  Otherwise it stays at the default level (currently 0).
    if (const char* sql_log_level = getenv(SP_LOG_SQL); sql_log_level) {
    	rodsLogSqlReq(1);
    }

    ServerBootTime = time( 0 );
    while ( ( c = getopt( argc, argv, "uvVqsh" ) ) != EOF ) {
        switch ( c ) {
        case 'u':               /* user command level. without serverized */
            uFlag = 1;
            break;
        case 'v':               /* verbose Logging */
            snprintf( tmpStr1, 100, "%s=%d", SP_LOG_LEVEL, LOG_NOTICE );
            putenv( tmpStr1 );
            rodsLogLevel( LOG_NOTICE );
            break;
        case 'V':               /* very Verbose */
            snprintf( tmpStr1, 100, "%s=%d", SP_LOG_LEVEL, LOG_DEBUG10 );
            putenv( tmpStr1 );
            rodsLogLevel( LOG_DEBUG10 );
            break;
        case 'q':               /* quiet (only errors and above) */
            snprintf( tmpStr1, 100, "%s=%d", SP_LOG_LEVEL, LOG_ERROR );
            putenv( tmpStr1 );
            rodsLogLevel( LOG_ERROR );
            break;
        case 's':               /* log SQL commands */
            snprintf( tmpStr2, 100, "%s=%d", SP_LOG_SQL, 1 );
            putenv( tmpStr2 );
            break;
        case 'h':               /* help */
            usage( argv[0] );
            exit( 0 );
        default:
            usage( argv[0] );
            exit( 1 );
        }
    }

    if (uFlag == 0) {
        daemonize();
    }

    /* start of irodsReServer has been moved to serverMain */
    signal( SIGTTIN, SIG_IGN );
    signal( SIGTTOU, SIG_IGN );
    signal( SIGCHLD, SIG_DFL ); /* SIG_IGN causes autoreap. wait get nothing */
    signal( SIGPIPE, SIG_IGN );
#ifdef osx_platform
    signal( SIGINT, ( sig_t ) serverExit );
    signal( SIGHUP, ( sig_t ) serverExit );
    signal( SIGTERM, ( sig_t ) serverExit );
#else
    signal( SIGINT, serverExit );
    signal( SIGHUP, serverExit );
    signal( SIGTERM, serverExit );
#endif

    // Set up local_addr for socket communication
    memset( &local_addr, 0, sizeof(local_addr) );
    local_addr.sun_family = AF_UNIX;
    char random_suffix[65];
    get64RandomBytes( random_suffix );

    char mkdtemp_template[sizeof(socket_dir_template)]{};
    snprintf(mkdtemp_template, sizeof(mkdtemp_template), "%s", socket_dir_template);

    const char* mkdtemp_result = mkdtemp(mkdtemp_template);
    if (!mkdtemp_result) {
        rodsLog(LOG_ERROR, "Error creating tmp directory for iRODS sockets, mkdtemp errno [%d]: [%s]", errno, strerror(errno));
        return SYS_INTERNAL_ERR;
    }
    snprintf(agent_factory_socket_dir, sizeof(agent_factory_socket_dir), "%s", mkdtemp_result);
    snprintf(agent_factory_socket_file, sizeof(agent_factory_socket_file), "%s/irods_factory_%s", agent_factory_socket_dir, random_suffix);
    snprintf(local_addr.sun_path, sizeof(local_addr.sun_path), "%s", agent_factory_socket_file);

    agent_spawning_pid = fork();

    if ( agent_spawning_pid == 0 ) {
        // Agent factory process (parent)
        ProcessType = AGENT_PT;
        return runIrodsAgentFactory( local_addr );
    } else if ( agent_spawning_pid > 0 ) {
        // Main iRODS server (grandparent)
        rodsLog( LOG_NOTICE, "Agent factory process pid = [%d]", agent_spawning_pid );

        // Create a UNIX domain socket and connect to the iRODS agent factory.
        agent_conn_socket = socket( AF_UNIX, SOCK_STREAM, 0 );

        time_t sock_connect_start_time = time( 0 );
        while (true) {
            const unsigned int len = sizeof(local_addr);
            ssize_t status = connect( agent_conn_socket, (const struct sockaddr*) &local_addr, len );
            if ( status >= 0 ) {
                break;
            }

            int saved_errno = errno;
            if ( ( time( 0 ) - sock_connect_start_time ) > 5 ) {
                rodsLog(LOG_ERROR, "Error connecting to agent factory socket, errno = [%d]: %s", saved_errno, strerror( saved_errno ) );
                return SYS_SOCK_CONNECT_ERR;
            }
        }
    } else {
        // Error, fork failed
        rodsLog( LOG_ERROR, "fork() failed when attempting to create agent factory process" );
        return SYS_FORK_ERROR;
    }

    return serverMain();
}

static bool instantiate_shared_memory_for_plugin( const std::unordered_map<std::string, boost::any>& _plugin_object ) {
    try {
        const auto& mem_name = boost::any_cast<const std::string&>(_plugin_object.at(irods::CFG_SHARED_MEMORY_INSTANCE_KW));
        prepareServerSharedMemory(mem_name);
        detachSharedMemory(mem_name);
    } catch ( const std::out_of_range& ) {
        return false;
    }
    return true;
}

static bool uninstantiate_shared_memory_for_plugin( const std::unordered_map<std::string, boost::any>& _plugin_object ) {
    try {
        const auto& mem_name = boost::any_cast<const std::string&>(_plugin_object.at(irods::CFG_SHARED_MEMORY_INSTANCE_KW));
        removeSharedMemory(mem_name);
        resetMutex(mem_name.c_str());
    } catch ( const std::out_of_range& ) {
        return false;
    }
    return true;
}

static irods::error instantiate_shared_memory( ) {
    try {
        for ( const auto& item : irods::get_server_property<const std::unordered_map<std::string, boost::any>&>(irods::CFG_PLUGIN_CONFIGURATION_KW) ) {
            if ( item.first == irods::PLUGIN_TYPE_RULE_ENGINE ) {
                for ( const auto& plugin : boost::any_cast<const std::vector<boost::any>&>(item.second) ) {
                    instantiate_shared_memory_for_plugin(boost::any_cast<const std::unordered_map<std::string, boost::any>&>(plugin));
                }
            } else {
                for ( const auto& plugin : boost::any_cast<const std::unordered_map<std::string, boost::any>&>(item.second) ) {
                    instantiate_shared_memory_for_plugin(boost::any_cast<const std::unordered_map<std::string, boost::any>&>(plugin.second));
                }
            }
        }
    } catch ( const boost::bad_any_cast& e ) {
        return ERROR(INVALID_ANY_CAST, e.what());
    } catch ( const irods::exception& e ) {
        return irods::error(e);
    }
    return SUCCESS();

} // instantiate_shared_memory

static irods::error uninstantiate_shared_memory( ) {
    try {
        for ( const auto& item : irods::get_server_property<const std::unordered_map<std::string, boost::any>&>(irods::CFG_PLUGIN_CONFIGURATION_KW) ) {
            if ( item.first == irods::PLUGIN_TYPE_RULE_ENGINE ) {
                for ( const auto& plugin : boost::any_cast<const std::vector<boost::any>&>(item.second) ) {
                    uninstantiate_shared_memory_for_plugin(boost::any_cast<const std::unordered_map<std::string, boost::any>&>(plugin));
                }
            } else {
                for ( const auto& plugin : boost::any_cast<const std::unordered_map<std::string, boost::any>&>(item.second) ) {
                    uninstantiate_shared_memory_for_plugin(boost::any_cast<const std::unordered_map<std::string, boost::any>&>(plugin.second));
                }
            }
        }
    } catch ( const boost::bad_any_cast& e ) {
        return ERROR(INVALID_ANY_CAST, e.what());
    } catch ( const irods::exception& e ) {
        return irods::error(e);
    }
    return SUCCESS();

} // uninstantiate_shared_memory

int
serverMain() {
    int acceptErrCnt = 0;

    // set re cache salt here
    irods::error ret = createAndSetRECacheSalt();
    if ( !ret.ok() ) {
        rodsLog( LOG_ERROR, "serverMain: createAndSetRECacheSalt error.\n%s", ret.result().c_str() );
        exit( 1 );
    }

    ret = instantiate_shared_memory();
    if(!ret.ok()) {
        irods::log(PASS(ret));
    }

    irods::re_plugin_globals.reset(new irods::global_re_plugin_mgr);

    rsComm_t svrComm;
    int status = initServerMain( &svrComm );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "serverMain: initServerMain error. status = %d",
                 status );
        exit( 1 );
    }

    std::string svc_role;
    ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    uint64_t return_code = 0;
    // =-=-=-=-=-=-=-
    // Launch the Control Plane
    try {
        irods::server_control_plane ctrl_plane(
            irods::CFG_SERVER_CONTROL_PLANE_PORT );

        status = startProcConnReqThreads();
        if(status < 0) {
            rodsLog(LOG_ERROR, "[%s] - Error in startProcConnReqThreads()", __FUNCTION__);
            return status;
        }
        if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            try {
                PurgeLockFileThread = new boost::thread( purgeLockFileWorkerTask );
            }
            catch ( const boost::thread_resource_error& ) {
                rodsLog( LOG_ERROR, "boost encountered a thread_resource_error during thread construction in serverMain." );
            }
        }

        fd_set sockMask;
        FD_ZERO( &sockMask );
        SvrSock = svrComm.sock;

        irods::server_state& server_state = irods::server_state::instance();
        while ( true ) {
            std::string the_server_state = server_state();
            if ( irods::server_state::STOPPED == the_server_state ) {
                procChildren( &ConnectedAgentHead );

                // Wake up the agent factory process so it can clean up and exit
                kill( agent_spawning_pid, SIGTERM );

                rodsLog(
                    LOG_NOTICE,
                    "iRODS Server is exiting with state [%s].",
                    the_server_state.c_str() );
                break;

            }
            else if ( irods::server_state::PAUSED == the_server_state ) {
                procChildren( &ConnectedAgentHead );
                rodsSleep(
                    0,
                    irods::SERVER_CONTROL_FWD_SLEEP_TIME_MILLI_SEC * 1000 );
                continue;

            }
            else {
                if ( irods::server_state::RUNNING != the_server_state ) {
                    rodsLog(
                        LOG_NOTICE,
                        "invalid iRODS server state [%s]",
                        the_server_state.c_str() );
                }

            }

            FD_SET( svrComm.sock, &sockMask );

            int numSock = 0;
            struct timeval time_out;
            time_out.tv_sec  = 0;
            time_out.tv_usec = irods::SERVER_CONTROL_POLLING_TIME_MILLI_SEC * 1000;
            while ( ( numSock = select(
                                    svrComm.sock + 1,
                                    &sockMask,
                                    ( fd_set * ) NULL,
                                    ( fd_set * ) NULL,
                                    &time_out ) ) < 0 ) {

                if ( errno == EINTR ) {
                    rodsLog( LOG_NOTICE, "serverMain: select() interrupted" );
                    FD_SET( svrComm.sock, &sockMask );
                    continue;
                }
                else {
                    rodsLog( LOG_NOTICE, "serverMain: select() error, errno = %d", errno );
                    return -1;
                }
            }

            procChildren( &ConnectedAgentHead );

            if ( 0 == numSock ) {
                continue;

            }

            const int newSock = rsAcceptConn( &svrComm );
            if ( newSock < 0 ) {
                acceptErrCnt++;
                if ( acceptErrCnt > MAX_ACCEPT_ERR_CNT ) {
                    rodsLog( LOG_ERROR, "serverMain: Too many socket accept error. Exiting" );
                    break;
                }
                else {
                    rodsLog( LOG_NOTICE, "serverMain: acceptConn() error, errno = %d", errno );
                    continue;
                }
            }
            else {
                acceptErrCnt = 0;
            }

            status = chkAgentProcCnt();
            if ( status < 0 ) {
                rodsLog( LOG_NOTICE,
                         "serverMain: chkAgentProcCnt failed status = %d", status );
                // =-=-=-=-=-=-=-
                // create network object to communicate to the network
                // plugin interface.  repave with newSock as that is the
                // operational socket at this point

                irods::network_object_ptr net_obj;
                irods::error ret = irods::network_factory( &svrComm, net_obj );
                if ( !ret.ok() ) {
                    irods::log( PASS( ret ) );
                }
                else {
                    ret = sendVersion( net_obj, status, 0, NULL, 0 );
                    if ( !ret.ok() ) {
                        irods::log( PASS( ret ) );
                    }
                }
                status = mySockClose( newSock );
                printf( "close status = %d\n", status );
                continue;
            }

            addConnReqToQue( &svrComm, newSock );
        }

        if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            try {
                PurgeLockFileThread->join();
            }
            catch ( const boost::thread_resource_error& ) {
                rodsLog( LOG_ERROR, "boost encountered a thread_resource_error during join in serverMain." );
            }
        }

        procChildren( &ConnectedAgentHead );
        stopProcConnReqThreads();

        server_state( irods::server_state::EXITED );
    }
    catch ( const irods::exception& e_ ) {
        rodsLog( LOG_ERROR, "Exception caught in server loop\n%s", e_.what() );
        return_code = e_.code();
    }

    uninstantiate_shared_memory();

    close( agent_conn_socket );
    unlink( agent_factory_socket_file );
    rmdir( agent_factory_socket_dir );

    rodsLog( LOG_NOTICE, "iRODS Server is done." );

    return return_code;
}

void
#if defined(linux_platform) || defined(aix_platform) || defined(solaris_platform) || defined(osx_platform)
serverExit( int sig )
#else
serverExit()
#endif
{
#if 0
    // RTS - rodsLog calls in signal handlers are unsafe - #3326 
    rodsLog( LOG_NOTICE, "rodsServer caught signal %d, exiting", sig );
#endif
    recordServerProcess( NULL ); /* unlink the process id file */

    close( agent_conn_socket );
    unlink( agent_factory_socket_file );
    rmdir( agent_factory_socket_dir );

    // Wake and terminate agent spawning process
    kill( agent_spawning_pid, SIGTERM );

    exit( 1 );
}

void
usage( char *prog ) {
    printf( "Usage: %s [-uvVqs]\n", prog );
    printf( " -u  user command level, remain attached to the tty (foreground)\n" );
    printf( " -v  verbose (LOG_NOTICE)\n" );
    printf( " -V  very verbose (LOG_DEBUG10)\n" );
    printf( " -q  quiet (LOG_ERROR)\n" );
    printf( " -s  log SQL commands\n" );
}

int
procChildren( agentProc_t **agentProcHead ) {
    agentProc_t *tmpAgentProc, *prevAgentProc, *finishedAgentProc;
    prevAgentProc = NULL;

    boost::unique_lock< boost::mutex > con_agent_lock( ConnectedAgentMutex );

    tmpAgentProc = *agentProcHead;

    while ( tmpAgentProc != NULL ) {
        // Check if pid is still an active process
        if ( kill( tmpAgentProc->pid, 0 ) ) {
            finishedAgentProc = tmpAgentProc;

            if ( prevAgentProc == NULL ) {
                *agentProcHead = tmpAgentProc->next;
            } else {
                prevAgentProc->next = tmpAgentProc->next;
            }
            tmpAgentProc = tmpAgentProc->next;
            free( finishedAgentProc );
        } else {
            prevAgentProc = tmpAgentProc;
            tmpAgentProc = tmpAgentProc->next;
        }
    }

    con_agent_lock.unlock();

    return 0;
}

agentProc_t *
getAgentProcByPid( int childPid, agentProc_t **agentProcHead ) {
    agentProc_t *tmpAgentProc, *prevAgentProc;
    prevAgentProc = NULL;

    boost::unique_lock< boost::mutex > con_agent_lock( ConnectedAgentMutex );

    tmpAgentProc = *agentProcHead;

    while ( tmpAgentProc != NULL ) {
        if ( childPid == tmpAgentProc->pid ) {
            if ( prevAgentProc == NULL ) {
                *agentProcHead = tmpAgentProc->next;
            }
            else {
                prevAgentProc->next = tmpAgentProc->next;
            }
            break;
        }
        prevAgentProc = tmpAgentProc;
        tmpAgentProc = tmpAgentProc->next;
    }

    con_agent_lock.unlock();

    return tmpAgentProc;
}

int
spawnAgent( agentProc_t *connReq, agentProc_t **agentProcHead ) {
    int childPid;
    int newSock;
    startupPack_t *startupPack;

    if ( connReq == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    newSock = connReq->sock;
    startupPack = &connReq->startupPack;

    childPid = execAgent( newSock, startupPack );
    if (childPid > 0) {
        queueConnectedAgentProc(childPid, connReq, agentProcHead);
    }

    return childPid;
}

int sendEnvironmentVarIntToSocket ( const char* var, int val, int socket ) {
    std::stringstream msg;
    msg << var << "=" << val << ";";
    ssize_t status = send( socket, msg.str().c_str(), msg.str().length(), 0 );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "Error in sendEnvironmentVarIntToSocket, errno = [%d]: %s", errno, strerror( errno ) );
    } else if ( static_cast<size_t>(status) != msg.str().length() ) {
        rodsLog( LOG_DEBUG, "Failed to send entire message in sendEnvironmentVarIntToSocket - msg [%s] is [%d] bytes long, sent [%d] bytes", msg.str().c_str(), msg.str().length(), status );
    }

    return status;
}

int sendEnvironmentVarStrToSocket ( const char* var, const char* val, int socket ) {
    std::stringstream msg;
    msg << var << "=" << val << ";";
    ssize_t status = send( socket, msg.str().c_str(), msg.str().length(), 0 );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "Error in sendEnvironmentVarIntToSocket, errno = [%d]: %s", errno, strerror( errno ) );
    } else if ( static_cast<size_t>(status) != msg.str().length() ) {
        rodsLog( LOG_DEBUG, "Failed to send entire message in sendEnvironmentVarIntToSocket - msg [%s] is [%d] bytes long, sent [%d] bytes", msg.str().c_str(), msg.str().length(), status );
    }

    return status;
}

ssize_t sendSocketOverSocket( int writeFd, int socket ) {
    struct msghdr msg;
    struct iovec iov[1];

    union {
        struct cmsghdr cm;
        char control[CMSG_SPACE(sizeof(int))];
    } control_un;
    struct cmsghdr *cmptr;

    memset( control_un.control, 0, sizeof(control_un.control) );
    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    cmptr = CMSG_FIRSTHDR(&msg);
    cmptr->cmsg_len = CMSG_LEN(sizeof(int));
    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;
    *((int *) CMSG_DATA(cmptr)) = socket;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    iov[0].iov_base = (void*) "i";
    iov[0].iov_len = 1;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    return sendmsg( writeFd, &msg, 0);
}

int
execAgent( int newSock, startupPack_t *startupPack ) {
    // Create unique socket for each call to exec agent
    char random_suffix[65]{};
    get64RandomBytes(random_suffix);

    sockaddr_un tmp_socket_addr{};
    char tmp_socket_file[sizeof(tmp_socket_addr.sun_path)]{};
    snprintf(tmp_socket_file, sizeof(tmp_socket_file), "%s/irods_agent_%s", agent_factory_socket_dir, random_suffix);

    ssize_t status{send(agent_conn_socket, tmp_socket_file, strlen(tmp_socket_file), 0)};
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "Error sending socket to agent factory process, errno = [%d]: %s", errno, strerror( errno ) );
    } else if ( static_cast<size_t>(status) < strlen( tmp_socket_file ) ) {
        rodsLog( LOG_DEBUG, "Failed to send entire message - msg [%s] is [%d] bytes long, sent [%d] bytes", tmp_socket_file, strlen( tmp_socket_file ), status );
    }

    tmp_socket_addr.sun_family = AF_UNIX;
    strncpy(tmp_socket_addr.sun_path, tmp_socket_file, sizeof(tmp_socket_addr.sun_path));

    int tmp_socket{socket(AF_UNIX, SOCK_STREAM, 0)};
    if ( tmp_socket < 0 ) {
        rodsLog( LOG_ERROR, "Unable to create socket in execAgent, errno = [%d]: %s", errno, strerror( errno ) );
    }

    const auto cleanup_sockets{[&]() {
        if (close(tmp_socket) < 0) {
            rodsLog(LOG_ERROR, "close(tmp_socket) failed with errno = [%d]: %s", errno, strerror(errno));
        }
        if (unlink(tmp_socket_file) < 0) {
            rodsLog(LOG_ERROR, "unlink(tmp_socket_file) failed with errno = [%d]: %s", errno, strerror(errno));
        }
    }};

    // Wait until receiving acknowledgement that socket has been created
    char in_buf[1024]{};
    status = recv( agent_conn_socket, &in_buf, sizeof(in_buf), 0 );
    if (status < 0) {
        rodsLog(LOG_ERROR, "Error in recv acknowledgement from agent factory process, errno = [%d]: %s", errno, strerror(errno));
        status = SYS_SOCK_READ_ERR;
    } else if (0 != strcmp(in_buf, "OK")) {
        rodsLog(LOG_ERROR, "Bad acknowledgement from agent factory process, message = [%s]", in_buf);
        status = SYS_SOCK_READ_ERR;
    }
    else {
        status = connect(tmp_socket, (const struct sockaddr*) &tmp_socket_addr, sizeof(local_addr));
        if (status < 0) {
            rodsLog(LOG_ERROR, "Unable to connect to socket in agent factory process, errno = [%d]: %s", errno, strerror(errno));
            status = SYS_SOCK_CONNECT_ERR;
        }
    }

    if (status < 0) {
        // Agent factory expects a message about connection to the agent - send failure
        const std::string failure_message{"spawn_failure"};
        send(agent_conn_socket, failure_message.c_str(), failure_message.length() + 1, 0);
        cleanup_sockets();
        return status;
    }
    else {
        // Notify agent factory of success and send data to agent process
        const std::string connection_successful{"connection_successful"};
        send(agent_conn_socket, connection_successful.c_str(), connection_successful.length() + 1, 0);
    }

    status = sendEnvironmentVarStrToSocket( SP_RE_CACHE_SALT,irods::get_server_property<const std::string>( irods::CFG_RE_CACHE_SALT_KW).c_str(),  tmp_socket );
    if (status < 0) {
        rodsLog( LOG_ERROR, "Failed to send SP_RE_CACHE_SALT to agent" );
    }
    status = sendEnvironmentVarIntToSocket( SP_CONNECT_CNT, startupPack->connectCnt, tmp_socket);
    if (status < 0) {
        rodsLog( LOG_ERROR, "Failed to send SP_CONNECT_CNT to agent" );
    }
    status = sendEnvironmentVarStrToSocket( SP_PROXY_RODS_ZONE, startupPack->proxyRodsZone, tmp_socket );
    if (status < 0) {
        rodsLog( LOG_ERROR, "Failed to send SP_PROXY_RODS_ZONE to agent" );
    }
    status = sendEnvironmentVarIntToSocket( SP_NEW_SOCK, newSock, tmp_socket );
    if (status < 0) {
        rodsLog( LOG_ERROR, "Failed to send SP_NEW_SOCK to agent" );
    }
    status = sendEnvironmentVarIntToSocket( SP_PROTOCOL, startupPack->irodsProt, tmp_socket );
    if (status < 0) {
        rodsLog( LOG_ERROR, "Failed to send SP_PROTOCOL to agent" );
    }
    status = sendEnvironmentVarIntToSocket( SP_RECONN_FLAG, startupPack->reconnFlag, tmp_socket );
    if (status < 0) {
        rodsLog( LOG_ERROR, "Failed to send SP_RECONN_FLAG to agent" );
    }
    status = sendEnvironmentVarStrToSocket( SP_PROXY_USER, startupPack->proxyUser, tmp_socket );
    if (status < 0) {
        rodsLog( LOG_ERROR, "Failed to send SP_PROXY_USER to agent" );
    }
    status = sendEnvironmentVarStrToSocket( SP_CLIENT_USER, startupPack->clientUser, tmp_socket );
    if (status < 0) {
        rodsLog( LOG_ERROR, "Failed to send SP_CLIENT_USER to agent" );
    }
    status = sendEnvironmentVarStrToSocket( SP_CLIENT_RODS_ZONE, startupPack->clientRodsZone, tmp_socket );
    if (status < 0) {
        rodsLog( LOG_ERROR, "Failed to send SP_CLIENT_RODS_ZONE to agent" );
    }
    status = sendEnvironmentVarStrToSocket( SP_REL_VERSION, startupPack->relVersion, tmp_socket );
    if (status < 0) {
        rodsLog( LOG_ERROR, "Failed to send SP_REL_VERSION to agent" );
    }
    status = sendEnvironmentVarStrToSocket( SP_API_VERSION, startupPack->apiVersion, tmp_socket );
    if (status < 0) {
        rodsLog( LOG_ERROR, "Failed to send SP_API_VERSION to agent" );
    }

    // =-=-=-=-=-=-=-
    // if the client-server negotiation request is in the
    // option variable, set that env var and strip it out
    std::string opt_str( startupPack->option );
    size_t pos = opt_str.find( REQ_SVR_NEG );
    if ( std::string::npos != pos ) {
        std::string trunc_str = opt_str.substr( 0, pos );
        status = sendEnvironmentVarStrToSocket( SP_OPTION, trunc_str.c_str(), tmp_socket );
        if (status < 0) {
            rodsLog( LOG_ERROR, "Failed to send SP_OPTION to agent" );
        }
        status = sendEnvironmentVarStrToSocket( irods::RODS_CS_NEG, REQ_SVR_NEG, tmp_socket );
        if (status < 0) {
            rodsLog( LOG_ERROR, "Failed to send irods::RODS_CS_NEG to agent" );
        }

    }
    else {
        status = sendEnvironmentVarStrToSocket( SP_OPTION, startupPack->option, tmp_socket );
        if (status < 0) {
            rodsLog( LOG_ERROR, "Failed to send SP_OPTION to agent" );
        }
    }

    status = sendEnvironmentVarIntToSocket( SERVER_BOOT_TIME, ServerBootTime, tmp_socket );
    if (status < 0) {
        rodsLog( LOG_ERROR, "Failed to send SERVER_BOOT_TIME to agent" );
    }

    status = send( tmp_socket, "end_of_vars", 12, 0 );
    if ( status <= 0 ) {
        rodsLog( LOG_ERROR, "Failed to send \"end_of_vars;\" to agent" );
    }

    status = recv( tmp_socket, &in_buf, sizeof(in_buf), 0 );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "Error in recv acknowledgement from agent factory process, errno = [%d]: %s", errno, strerror( errno ) );
        cleanup_sockets();
        return SYS_SOCK_READ_ERR;
    } else if ( strcmp(in_buf, "OK") != 0 ) {
        rodsLog( LOG_ERROR, "Bad acknowledgement from agent factory process, message = [%s]", in_buf );
        cleanup_sockets();
        return SYS_SOCK_READ_ERR;
    }
    sendSocketOverSocket( tmp_socket, newSock );
    status = recv( tmp_socket, &in_buf, sizeof(in_buf), 0 );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "Error in recv child_pid from agent factory process, errno = [%d]: %s", errno, strerror( errno ) );
        cleanup_sockets();
        return SYS_SOCK_READ_ERR;
    }

    cleanup_sockets();
    return std::atoi(in_buf);
}

int
queueConnectedAgentProc( int childPid, agentProc_t *connReq,
                       agentProc_t **agentProcHead ) {
    if ( connReq == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    connReq->pid = childPid;

    boost::unique_lock< boost::mutex > con_agent_lock( ConnectedAgentMutex );

    queueAgentProc( connReq, agentProcHead, TOP_POS );

    con_agent_lock.unlock();

    return 0;
}

int
getAgentProcCnt() {
    agentProc_t *tmpAgentProc;
    int count = 0;

    boost::unique_lock< boost::mutex > con_agent_lock( ConnectedAgentMutex );

    tmpAgentProc = ConnectedAgentHead;
    while ( tmpAgentProc != NULL ) {
        count++;
        tmpAgentProc = tmpAgentProc->next;
    }
    con_agent_lock.unlock();

    return count;
}

int getAgentProcPIDs(
    std::vector<int>& _pids ) {
    agentProc_t *tmp_proc = 0;
    int count = 0;

    boost::unique_lock< boost::mutex > con_agent_lock( ConnectedAgentMutex );

    tmp_proc = ConnectedAgentHead;
    while ( tmp_proc != NULL ) {
        count++;
        _pids.push_back( tmp_proc->pid );
        tmp_proc = tmp_proc->next;
    }
    con_agent_lock.unlock();

    return count;

} // getAgentProcPIDs

int
chkAgentProcCnt() {
    int maximum_connections;
    try {
        maximum_connections = irods::get_server_property<const int>("maximum_connections");
    } catch ( const irods::exception& e ) {
        if ( e.code() == KEY_NOT_FOUND ) {
            maximum_connections = NO_MAX_CONNECTION_LIMIT;
        } else {
            irods::log( irods::error(e) );
            return e.code();
        }
    }
    if ( maximum_connections != NO_MAX_CONNECTION_LIMIT ) {
        int count = getAgentProcCnt();
        if ( count >= maximum_connections ) {
            chkConnectedAgentProcQue();
            count = getAgentProcCnt();
            if ( count >= maximum_connections ) {
                return SYS_MAX_CONNECT_COUNT_EXCEEDED;
            }
        }
    }
    return 0;
}

int
chkConnectedAgentProcQue() {
    agentProc_t *tmpAgentProc, *prevAgentProc, *unmatchedAgentProc;
    prevAgentProc = NULL;

    boost::unique_lock< boost::mutex > con_agent_lock( ConnectedAgentMutex );
    tmpAgentProc = ConnectedAgentHead;

    while ( tmpAgentProc != NULL ) {
        char procPath[MAX_NAME_LEN];

        snprintf( procPath, MAX_NAME_LEN, "%s/%-d", ProcLogDir, tmpAgentProc->pid );
        path p( procPath );
        if ( !exists( p ) ) {
            /* the agent proc is gone */
            unmatchedAgentProc = tmpAgentProc;
            rodsLog( LOG_DEBUG,
                     "Agent process %d in Connected queue but not in ProcLogDir",
                     tmpAgentProc->pid );
            if ( prevAgentProc == NULL ) {
                ConnectedAgentHead = tmpAgentProc->next;
            }
            else {
                prevAgentProc->next = tmpAgentProc->next;
            }
            tmpAgentProc = tmpAgentProc->next;
            free( unmatchedAgentProc );
        }
        else {
            prevAgentProc = tmpAgentProc;
            tmpAgentProc = tmpAgentProc->next;
        }
    }
    con_agent_lock.unlock();

    return 0;
}

int
initServer( rsComm_t *svrComm ) {
    int status;
    rodsServerHost_t *rodsServerHost = NULL;

    status = initServerInfo( 0, svrComm );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "initServer: initServerInfo error, status = %d",
                 status );
        return status;
    }

    // JMC - legacy resources - printLocalResc ();
    resc_mgr.print_local_resources();

    printZoneInfo();

    status = getRcatHost( MASTER_RCAT, NULL, &rodsServerHost );

    if ( status < 0 || NULL == rodsServerHost ) { // JMC cppcheck - nullptr
        return status;
    }

    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }


    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
        if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            disconnectRcat();
        }
    }
    else {
        if ( rodsServerHost->conn != NULL ) {
            rcDisconnect( rodsServerHost->conn );
            rodsServerHost->conn = NULL;
        }
    }

    if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        purgeLockFileDir( 0 );
    }

    return status;
}

/* record the server process number and other information into
   a well-known file.  If svrComm is Null and this has created a file
   before, just unlink the file. */
int
recordServerProcess( rsComm_t *svrComm ) {
    int myPid;
    FILE *fd;
    DIR  *dirp;
    static char filePath[100] = "";
    char cwd[1000];
    char stateFile[] = "irodsServer";
    char *tmp;
    char *cp;

    if ( svrComm == NULL ) {
        if ( filePath[0] != '\0' ) {
            unlink( filePath );
        }
        return 0;
    }
    rodsEnv *myEnv = &( svrComm->myEnv );

    /* Use /usr/tmp if it exists, /tmp otherwise */
    dirp = opendir( "/usr/tmp" );
    if ( dirp != NULL ) {
        tmp = "/usr/tmp";
        ( void )closedir( dirp );
    }
    else {
        tmp = "/tmp";
    }

    sprintf( filePath, "%s/%s.%d", tmp, stateFile, myEnv->rodsPort );

    unlink( filePath );

    myPid = getpid();
    cp = getcwd( cwd, 1000 );
    if ( cp != NULL ) {
        fd = fopen( filePath, "w" );
        if ( fd != NULL ) {
            fprintf( fd, "%d %s\n", myPid, cwd );
            fclose( fd );
            int err_code = chmod( filePath, 0664 );
            if ( err_code != 0 ) {
                rodsLog( LOG_ERROR, "chmod failed in recordServerProcess on [%s] with error code %d", filePath, err_code );
            }
        }
    }
    return 0;
}

int
initServerMain( rsComm_t *svrComm ) {
    memset( svrComm, 0, sizeof( *svrComm ) );
    int status = getRodsEnv( &svrComm->myEnv );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "initServerMain: getRodsEnv error. status = %d",
                 status );
        return status;
    }
    initAndClearProcLog();

    setRsCommFromRodsEnv( svrComm );

    status = initServer( svrComm );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "initServerMain: initServer error. status = %d",
                 status );
        exit( 1 );
    }


    int zone_port;
    try {
        zone_port = irods::get_server_property<const int>(irods::CFG_ZONE_PORT);
    } catch ( irods::exception& e ) {
        irods::log( irods::error(e) );
        return e.code();
    }
    svrComm->sock = sockOpenForInConn(
            svrComm,
            &zone_port,
            NULL,
            SOCK_STREAM );
    if ( svrComm->sock < 0 ) {
        rodsLog( LOG_ERROR,
                "initServerMain: sockOpenForInConn error. status = %d",
                svrComm->sock );
        return svrComm->sock;
    }

    if ( listen( svrComm->sock, MAX_LISTEN_QUE ) < 0 ) {
        rodsLog( LOG_ERROR,
                 "initServerMain: listen failed, errno: %d",
                 errno );
        return SYS_SOCK_LISTEN_ERR;
    }

    rodsLog( LOG_NOTICE,
             "rodsServer Release version %s - API Version %s is up",
             RODS_REL_VERSION, RODS_API_VERSION );

    /* Record port, pid, and cwd into a well-known file */
    recordServerProcess( svrComm );
    /* start the irodsReServer */
    rodsServerHost_t *reServerHost = NULL;
    getReHost( &reServerHost );
    if ( reServerHost != NULL && reServerHost->localFlag == LOCAL_HOST ) {
        int re_pid = RODS_FORK();
        if ( re_pid == 0 ) { // child

            close( svrComm->sock );
            std::vector<char *> av;
            av.push_back( "irodsReServer" );
            av.push_back( NULL );
            rodsLog( LOG_NOTICE, "Starting irodsReServer" );
            execv( av[0], &av[0] );
            exit( 1 );
        }
        else {
            irods::set_server_property<int>( irods::RE_PID_KW, re_pid );
        }
    }

    return 0;
}

/* add incoming connection request to the bottom of the link list */

int
addConnReqToQue( rsComm_t *rsComm, int sock ) {
    agentProc_t *myConnReq;

    boost::unique_lock< boost::mutex > read_req_lock( ReadReqCondMutex );
    myConnReq = ( agentProc_t* )calloc( 1, sizeof( agentProc_t ) );

    myConnReq->sock = sock;
    myConnReq->remoteAddr = rsComm->remoteAddr;
    queueAgentProc( myConnReq, &ConnReqHead, BOTTOM_POS );

    ReadReqCond.notify_all(); // NOTE:: check all vs one
    read_req_lock.unlock();

    return 0;
}

int
initConnThreadEnv() {
    return 0;
}

agentProc_t *
getConnReqFromQue() {
    agentProc_t *myConnReq = NULL;

    irods::server_state& server_state = irods::server_state::instance();
    while ( irods::server_state::STOPPED != server_state() &&
            irods::server_state::EXITED != server_state() &&
            myConnReq == NULL ) {
        boost::unique_lock<boost::mutex> read_req_lock( ReadReqCondMutex );
        if ( ConnReqHead != NULL ) {
            myConnReq = ConnReqHead;
            ConnReqHead = ConnReqHead->next;
            read_req_lock.unlock();
            break;
        }

        ReadReqCond.wait( read_req_lock );
        if ( ConnReqHead == NULL ) {
            read_req_lock.unlock();
            continue;
        }
        else {
            myConnReq = ConnReqHead;
            ConnReqHead = ConnReqHead->next;
            read_req_lock.unlock();
            break;
        }
    }

    return myConnReq;
}

int
startProcConnReqThreads() {
    initConnThreadEnv();
    for ( int i = 0; i < NUM_READ_WORKER_THR; i++ ) {
        try {
            ReadWorkerThread[i] = new boost::thread( readWorkerTask );
        }
        catch ( const boost::thread_resource_error& ) {
            rodsLog( LOG_ERROR, "boost encountered a thread_resource_error during thread construction in startProcConnReqThreads." );
            return SYS_THREAD_RESOURCE_ERR;
        }
    }
    try {
        SpawnManagerThread = new boost::thread( spawnManagerTask );
    }
    catch ( const boost::thread_resource_error& ) {
        rodsLog( LOG_ERROR, "boost encountered a thread_resource_error during thread construction in startProcConnReqThreads." );
        return SYS_THREAD_RESOURCE_ERR;
    }

    return 0;
}

void
stopProcConnReqThreads() {

    SpawnReqCond.notify_all();
    try {
        SpawnManagerThread->join();
    }
    catch ( const boost::thread_resource_error& ) {
        rodsLog( LOG_ERROR, "boost encountered a thread_resource_error during join in stopProcConnReqThreads." );
    }

    for ( int i = 0; i < NUM_READ_WORKER_THR; i++ ) {
        ReadReqCond.notify_all();
        try {
            ReadWorkerThread[i]->join();
        }
        catch ( const boost::thread_resource_error& ) {
            rodsLog( LOG_ERROR, "boost encountered a thread_resource_error during join in stopProcConnReqThreads." );
        }
    }
}
void
readWorkerTask() {

    // =-=-=-=-=-=-=-
    // artificially create a conn object in order to
    // create a network object.  this is gratuitous
    // but necessary to maintain the consistent interface.
    rcComm_t            tmp_comm;
    bzero( &tmp_comm, sizeof( rcComm_t ) );

    irods::network_object_ptr net_obj;
    irods::error ret = irods::network_factory( &tmp_comm, net_obj );
    if ( !ret.ok() || !net_obj.get() ) {
        irods::log( PASS( ret ) );
        return;
    }

    irods::server_state& server_state = irods::server_state::instance();
    while ( irods::server_state::STOPPED != server_state() &&
            irods::server_state::EXITED != server_state() ) {
        agentProc_t *myConnReq = getConnReqFromQue();
        if ( myConnReq == NULL ) {
            /* someone else took care of it */
            continue;
        }
        int newSock = myConnReq->sock;

        // =-=-=-=-=-=-=-
        // repave the socket handle with the new socket
        // for this connection
        net_obj->socket_handle( newSock );
        startupPack_t *startupPack = nullptr;
        struct timeval tv;
        tv.tv_sec = READ_STARTUP_PACK_TOUT_SEC;
        tv.tv_usec = 0;
        irods::error ret = readStartupPack( net_obj, &startupPack, &tv );

        if ( !ret.ok() ) {
            rodsLog( LOG_ERROR, "readWorkerTask - readStartupPack failed. %d", ret.code() );
            sendVersion( net_obj, ret.code(), 0, NULL, 0 );
            boost::unique_lock<boost::mutex> bad_req_lock( BadReqMutex );
            queueAgentProc( myConnReq, &BadReqHead, TOP_POS );
            bad_req_lock.unlock();
            mySockClose( newSock );
        }
        else if ( strcmp(startupPack->option, RODS_HEARTBEAT_T) == 0 ) {
            const char* heartbeat = RODS_HEARTBEAT_T;
            const int heartbeat_length = strlen(heartbeat);
            int bytes_to_send = heartbeat_length;
            while ( bytes_to_send ) {
                const int bytes_sent = send(newSock, &(heartbeat[heartbeat_length - bytes_to_send]), bytes_to_send, 0);
                const int errsav = errno;
                if ( bytes_sent > 0 ) {
                    bytes_to_send -= bytes_sent;
                } else if ( errsav != EINTR ) {
                    rodsLog(LOG_ERROR, "Socket error encountered during heartbeat; socket returned %s", strerror(errsav));
                    break;
                }
            }
            mySockClose(newSock);
            free( myConnReq );
            free( startupPack );
        }
        else if ( startupPack->connectCnt > MAX_SVR_SVR_CONNECT_CNT ) {
            sendVersion( net_obj, SYS_EXCEED_CONNECT_CNT, 0, NULL, 0 );
            mySockClose( newSock );
            free( myConnReq );
            free( startupPack );
        }
        else {
            if ( startupPack->clientUser[0] == '\0' ) {
                int status = chkAllowedUser( startupPack->clientUser,
                                             startupPack->clientRodsZone );
                if ( status < 0 ) {
                    sendVersion( net_obj, status, 0, NULL, 0 );
                    mySockClose( newSock );
                    free( myConnReq );
                    continue;
                }
            }

            myConnReq->startupPack = *startupPack;
            free( startupPack );

            boost::unique_lock< boost::mutex > spwn_req_lock( SpawnReqCondMutex );

            queueAgentProc( myConnReq, &SpawnReqHead, BOTTOM_POS );

            SpawnReqCond.notify_all(); // NOTE:: look into notify_one vs notify_all

        } // else

    } // while 1

} // readWorkerTask

void
spawnManagerTask() {
    agentProc_t *mySpawnReq = NULL;
    int status;
    uint curTime;
    uint agentQueChkTime = 0;

    irods::server_state& server_state = irods::server_state::instance();
    while ( irods::server_state::STOPPED != server_state() &&
            irods::server_state::EXITED != server_state() ) {

        boost::unique_lock<boost::mutex> spwn_req_lock( SpawnReqCondMutex );
        SpawnReqCond.wait( spwn_req_lock );

        while ( SpawnReqHead != NULL ) {
            mySpawnReq = SpawnReqHead;
            SpawnReqHead = mySpawnReq->next;

            spwn_req_lock.unlock();
            status = spawnAgent( mySpawnReq, &ConnectedAgentHead );
            close( mySpawnReq->sock );

            if ( status < 0 ) {
                rodsLog( LOG_NOTICE,
                         "spawnAgent error for puser=%s and cuser=%s from %s, stat=%d",
                         mySpawnReq->startupPack.proxyUser,
                         mySpawnReq->startupPack.clientUser,
                         inet_ntoa( mySpawnReq->remoteAddr.sin_addr ), status );
                free( mySpawnReq );
            }
            else {
                rodsLog( LOG_DEBUG,
                         "Agent process %d started for puser=%s and cuser=%s from %s",
                         mySpawnReq->pid, mySpawnReq->startupPack.proxyUser,
                         mySpawnReq->startupPack.clientUser,
                         inet_ntoa( mySpawnReq->remoteAddr.sin_addr ) );
            }

            spwn_req_lock.lock();

        }

        spwn_req_lock.unlock();

        curTime = time( 0 );
        if ( curTime > agentQueChkTime + AGENT_QUE_CHK_INT ) {
            agentQueChkTime = curTime;
            procBadReq();
        }
    }
}

int
procSingleConnReq( agentProc_t *connReq ) {
    if ( connReq == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    int newSock = connReq->sock;

    // =-=-=-=-=-=-=-
    // artificially create a conn object in order to
    // create a network object.  this is gratuitous
    // but necessary to maintain the consistent interface.
    rcComm_t            tmp_comm;
    bzero( &tmp_comm, sizeof( rcComm_t ) );

    irods::network_object_ptr net_obj;
    irods::error ret = irods::network_factory( &tmp_comm, net_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return -1;
    }

    net_obj->socket_handle( newSock );

    startupPack_t *startupPack;
    ret = readStartupPack( net_obj, &startupPack, NULL );

    if ( !ret.ok() ) {
        rodsLog( LOG_NOTICE, "readStartupPack error from %s, status = %d",
                 inet_ntoa( connReq->remoteAddr.sin_addr ), ret.code() );
        sendVersion( net_obj, ret.code(), 0, NULL, 0 );
        mySockClose( newSock );
        return ret.code();
    }

    if ( startupPack->connectCnt > MAX_SVR_SVR_CONNECT_CNT ) {
        sendVersion( net_obj, SYS_EXCEED_CONNECT_CNT, 0, NULL, 0 );
        mySockClose( newSock );
        return SYS_EXCEED_CONNECT_CNT;
    }

    connReq->startupPack = *startupPack;
    free( startupPack );

    int status = spawnAgent( connReq, &ConnectedAgentHead );

    close( newSock );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "spawnAgent error for puser=%s and cuser=%s from %s, status = %d",
                 connReq->startupPack.proxyUser, connReq->startupPack.clientUser,
                 inet_ntoa( connReq->remoteAddr.sin_addr ), status );
    }
    else {
        rodsLog( LOG_DEBUG,
                 "Agent process %d started for puser=%s and cuser=%s from %s",
                 connReq->pid, connReq->startupPack.proxyUser,
                 connReq->startupPack.clientUser,
                 inet_ntoa( connReq->remoteAddr.sin_addr ) );
    }
    return status;
}

/* procBadReq - process bad request */
int
procBadReq() {
    agentProc_t *tmpConnReq, *nextConnReq;
    /* just free them for now */

    boost::unique_lock< boost::mutex > bad_req_lock( BadReqMutex );

    tmpConnReq = BadReqHead;
    while ( tmpConnReq != NULL ) {
        nextConnReq = tmpConnReq->next;
        free( tmpConnReq );
        tmpConnReq = nextConnReq;
    }
    BadReqHead = NULL;
    bad_req_lock.unlock();

    return 0;
}

// =-=-=-=-=-=-=-
// JMC - backport 4612
void
purgeLockFileWorkerTask() {
    size_t wait_time_ms = 0;
    const size_t purge_time_ms = LOCK_FILE_PURGE_TIME * 1000; // s to ms

    irods::server_state& server_state = irods::server_state::instance();
    while ( irods::server_state::STOPPED != server_state() &&
            irods::server_state::EXITED != server_state() ) {
        rodsSleep( 0, irods::SERVER_CONTROL_POLLING_TIME_MILLI_SEC * 1000 ); // second, microseconds
        wait_time_ms += irods::SERVER_CONTROL_POLLING_TIME_MILLI_SEC;

        if ( wait_time_ms >= purge_time_ms ) {
            wait_time_ms = 0;
            int status = purgeLockFileDir( 1 );
            if ( status < 0 ) {
                rodsLogError(
                    LOG_ERROR,
                    status,
                    "purgeLockFileWorkerTask: purgeLockFileDir failed" );
            }

        } // if

    } // while

} // purgeLockFileWorkerTask

std::vector<std::string>
setExecArg( const char *commandArgv ) {

    std::vector<std::string> arguments;
    if ( commandArgv != NULL ) {
        int len = 0;
        bool openQuote = false;
        const char* cur = commandArgv;
        for ( int i = 0; commandArgv[i] != '\0'; i++ ) {
            if ( commandArgv[i] == ' ' && !openQuote ) {
                if ( len > 0 ) {    /* end of a argv */
                    std::string( cur, len );
                    arguments.push_back( std::string( cur, len ) );
                    /* reset inx and pointer */
                    cur = &commandArgv[i + 1];
                    len = 0;
                }
                else {      /* skip over blanks */
                    cur = &commandArgv[i + 1];
                }
            }
            else if ( commandArgv[i] == '\'' || commandArgv[i] == '\"' ) {
                openQuote ^= true;
                if ( openQuote ) {
                    /* skip the quote */
                    cur = &commandArgv[i + 1];
                }
            }
            else {
                len ++;
            }
        }
        if ( len > 0 ) {    /* handle the last argv */
            arguments.push_back( std::string( cur, len ) );
        }
    }

    return arguments;
}
// =-=-=-=-=-=-=-
