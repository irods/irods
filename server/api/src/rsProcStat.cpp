#include "irods/irods_default_paths.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/rcMisc.h"
#include "irods/procStat.h"
#include "irods/objMetaOpr.hpp"
#include "irods/resource.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/rodsLog.h"
#include "irods/rsGlobalExtern.hpp"
#include "irods/rcGlobalExtern.h"
#include "irods/rsProcStat.hpp"
#include "irods/irods_resource_backport.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/lexical_cast.hpp>

#include <cstring>

namespace
{
    using log_api = irods::experimental::log::api;

    int readProcLog(int pid, procLog_t* procLog)
    {
        if (nullptr == procLog) {
            log_api::error("{}: Invalid input argument: nullptr", __func__);
            return USER__NULL_INPUT_ERR;
        }

        const auto proc_dir = irods::get_irods_proc_directory();
        const auto agent_info_path = proc_dir / std::to_string(pid);

        std::ifstream procStream{agent_info_path.c_str()};
        std::vector<std::string> procTokens;
        while (!procStream.eof() && procTokens.size() < 7) {
            std::string token;
            procStream >> token;
            log_api::debug("{}: Adding token to agent proc info: [{}]", __func__, token);
            procTokens.push_back(std::move(token));
        }

        if (procTokens.size() != 7) {
            log_api::error("{}: Agent process info: [{}], number of parameters: [{}]", __func__, agent_info_path.c_str(), procTokens.size());
            return UNIX_FILE_READ_ERR;
        }

        procLog->pid = pid;

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        std::snprintf(procLog->clientName, sizeof(procLog->clientName), "%s", procTokens[0].c_str());
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        std::snprintf(procLog->clientZone, sizeof(procLog->clientZone), "%s", procTokens[1].c_str());
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        std::snprintf(procLog->proxyName, sizeof(procLog->proxyName), "%s", procTokens[2].c_str());
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        std::snprintf(procLog->proxyZone, sizeof(procLog->proxyZone), "%s", procTokens[3].c_str());
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        std::snprintf(procLog->progName, sizeof(procLog->progName), "%s", procTokens[4].c_str());
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        std::snprintf(procLog->remoteAddr, sizeof(procLog->remoteAddr), "%s", procTokens[5].c_str());

        try {
            procLog->startTime = boost::lexical_cast<unsigned int>(procTokens[6].c_str());
        }
        catch (const std::exception& e) {
            log_api::error("{}: Could not convert [{}] to unsigned int.", __func__, procTokens[6]);
            return INVALID_LEXICAL_CAST;
        }

        return 0;
    } // readProcLog
} // anonymous namespace

int rsProcStat( rsComm_t * rsComm, procStatInp_t * procStatInp, genQueryOut_t **procStatOut ) {
    if (*procStatInp->rodsZone != '\0') {
        rodsServerHost_t* rodsServerHost = nullptr;
        const auto remoteFlag = getRcatHost(PRIMARY_RCAT, procStatInp->rodsZone, &rodsServerHost);
        if ( remoteFlag < 0 ) {
            log_api::error("{}: getRcatHost() failed. error={}", __func__, remoteFlag);
            return remoteFlag;
        }

        if ( rodsServerHost->localFlag == REMOTE_HOST ) {
            return remoteProcStat( rsComm, procStatInp, procStatOut, rodsServerHost );
        }

        return _rsProcStat( rsComm, procStatInp, procStatOut );
    }

    return _rsProcStat( rsComm, procStatInp, procStatOut );
}

int
_rsProcStat( rsComm_t *rsComm, procStatInp_t *procStatInp,
             genQueryOut_t **procStatOut ) {
    int status = -1;
    rodsServerHost_t *rodsServerHost = NULL;
    int remoteFlag = -1;
    rodsHostAddr_t addr;
    procStatInp_t myProcStatInp;
    char *tmpStr = NULL;

    if ( getValByKey( &procStatInp->condInput, ALL_KW ) != NULL ) {
        status = _rsProcStatAll( rsComm, procStatOut );
        return status;
    }
    if ( getValByKey( &procStatInp->condInput, EXEC_LOCALLY_KW ) != NULL ) {
        status = localProcStat( procStatInp, procStatOut );
        return status;
    }

    std::memset(&addr, 0, sizeof(addr));
    std::memset(&myProcStatInp, 0, sizeof(myProcStatInp));
    if ( *procStatInp->addr != '\0' ) {	/* given input addr */
        rstrcpy( addr.hostAddr, procStatInp->addr, LONG_NAME_LEN );
        remoteFlag = resolveHost( &addr, &rodsServerHost );
    }
    else if ( ( tmpStr = getValByKey( &procStatInp->condInput, RESC_NAME_KW ) ) != NULL ) {
        std::string resc_name( tmpStr );
        irods::error ret = SUCCESS();

        rodsLong_t resc_id = 0;
        ret = resc_mgr.hier_to_leaf_id(resc_name,resc_id);
        if(!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }

        // Get resource location
        std::string resc_loc;
        ret = irods::get_resource_property< std::string >( resc_id, irods::RESOURCE_LOCATION, resc_loc );
        if ( !ret.ok() ) {
            irods::log PASSMSG( "_rsProcStat - failed in get_resource_property", ret );
            return ret.code();
        }
        snprintf( procStatInp->addr, NAME_LEN, "%s", resc_loc.c_str() );

        // Get resource host
        std::string resc_host;
        ret = irods::get_resource_property< rodsServerHost_t* >( resc_id, irods::RESOURCE_HOST, rodsServerHost );
        if ( !ret.ok() ) {
            irods::log PASSMSG( "_rsProcStat - failed in get_resource_property", ret );
            return ret.code();
        }

        if ( !rodsServerHost ) {
            remoteFlag = SYS_INVALID_SERVER_HOST;
        }
        else {
            remoteFlag = rodsServerHost->localFlag;
        }
    }
    else {
        /* do the IES server */
        remoteFlag = getRcatHost(PRIMARY_RCAT, NULL, &rodsServerHost);
    }
    if ( remoteFlag < 0 ) {
        rodsLog( LOG_ERROR,
                 "_rsProcStat: getRcatHost() failed. error=%d", remoteFlag );
        return remoteFlag;
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        addKeyVal( &myProcStatInp.condInput, EXEC_LOCALLY_KW, "" );
        status = remoteProcStat( rsComm, &myProcStatInp, procStatOut,
                                 rodsServerHost );
        rmKeyVal( &myProcStatInp.condInput, EXEC_LOCALLY_KW );
    }
    else {
        status = localProcStat( procStatInp, procStatOut );
    }
    return status;
}

int
_rsProcStatAll( rsComm_t *rsComm,
                genQueryOut_t **procStatOut ) {
    rodsServerHost_t *tmpRodsServerHost;
    procStatInp_t myProcStatInp;
    int status;
    genQueryOut_t *singleProcStatOut = NULL;
    int savedStatus = 0;

    std::memset(&myProcStatInp, 0, sizeof(myProcStatInp));
    tmpRodsServerHost = ServerHostHead;
    while ( tmpRodsServerHost != NULL ) {
        irods::error err = irods::get_host_status_by_host_info( tmpRodsServerHost );
        if ( err.ok() && err.code() == INT_RESC_STATUS_UP ) {
            if ( tmpRodsServerHost->localFlag == LOCAL_HOST ) {
                setLocalSrvAddr( myProcStatInp.addr );
                status = localProcStat( &myProcStatInp, &singleProcStatOut );
            }
            else {
                rstrcpy( myProcStatInp.addr, tmpRodsServerHost->hostName->name, NAME_LEN );
                addKeyVal( &myProcStatInp.condInput, EXEC_LOCALLY_KW, "" );
                status = remoteProcStat( rsComm, &myProcStatInp, &singleProcStatOut, tmpRodsServerHost );
                rmKeyVal( &myProcStatInp.condInput, EXEC_LOCALLY_KW );
            }

            if ( status < 0 ) {
                savedStatus = status;
            }

            if ( singleProcStatOut != NULL ) {
                if ( *procStatOut == NULL ) {
                    *procStatOut = singleProcStatOut;
                }
                else {
                    catGenQueryOut( *procStatOut, singleProcStatOut, MAX_PROC_STAT_CNT );
                    freeGenQueryOut( &singleProcStatOut );
                }
                singleProcStatOut = NULL;
            }

        } // if up

        tmpRodsServerHost = tmpRodsServerHost->next;
    } // while
    return savedStatus;
}

int
localProcStat( procStatInp_t *procStatInp,
               genQueryOut_t **procStatOut ) {
    int numProc, status = 0;
    procLog_t procLog;
    char childPath[MAX_NAME_LEN];
    int count = 0;

    const auto proc_dir = irods::get_irods_proc_directory();
    numProc = getNumFilesInDir(proc_dir.c_str()) + 2; /* add 2 to give some room */

    std::memset(&procLog, 0, sizeof(procLog));
    /* init serverAddr */
    if ( *procStatInp->addr != '\0' ) { /* given input addr */
        rstrcpy( procLog.serverAddr, procStatInp->addr, NAME_LEN );
    }
    else {
        setLocalSrvAddr( procLog.serverAddr );
    }
    if ( numProc <= 0 ) {
        /* add an empty entry with only serverAddr */
        initProcStatOut( procStatOut, 1 );
        addProcToProcStatOut( &procLog, *procStatOut );
        return numProc;
    }
    else {
        initProcStatOut( procStatOut, numProc );
    }

    /* loop through the directory */
    namespace fs = boost::filesystem;
    fs::path srcDirPath(proc_dir);
    if ( !fs::exists( srcDirPath ) || !fs::is_directory( srcDirPath ) ) {
        status = USER_INPUT_PATH_ERR - errno;
        rodsLogError( LOG_ERROR, status,
                      "localProcStat: opendir local dir error for %s", proc_dir.c_str());
        return status;
    }
    fs::directory_iterator end_itr; // default construction yields past-the-end
    for ( fs::directory_iterator itr( srcDirPath ); itr != end_itr; ++itr ) {
        fs::path p = itr->path();
        fs::path cp = p.filename();
        if ( !isdigit( *cp.c_str() ) ) {
            continue;    /* not a pid */
        }
        snprintf( childPath, MAX_NAME_LEN, "%s",
                  p.c_str() );
        if ( !exists( p ) ) {
            rodsLogError( LOG_ERROR, status,
                          "localProcStat: stat error for %s", childPath );
            continue;
        }
        if ( is_regular_file( p ) ) {
            if ( count >= numProc ) {
                rodsLog( LOG_ERROR,
                         "localProcStat: proc count %d exceeded", numProc );
                break;
            }
            procLog.pid = atoi( cp.c_str() );
            if ( readProcLog( procLog.pid, &procLog ) < 0 ) {
                continue;
            }
            status = addProcToProcStatOut( &procLog, *procStatOut );
            if ( status < 0 ) {
                continue;
            }
            count++;
        }
        else {
            continue;
        }
    }
    return 0;
}

int
remoteProcStat( rsComm_t *rsComm, procStatInp_t *procStatInp,
                genQueryOut_t **procStatOut, rodsServerHost_t *rodsServerHost ) {
    int status;
    procLog_t procLog;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_ERROR,
                 "remoteProcStat: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( procStatInp == NULL || procStatOut == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    status = svrToSvrConnect( rsComm, rodsServerHost );

    if ( status >= 0 ) {
        status = rcProcStat( rodsServerHost->conn, procStatInp, procStatOut );
    }
    if ( status < 0 && *procStatOut == NULL ) {
        /* add an empty entry */
        initProcStatOut( procStatOut, 1 );
        std::memset(&procLog, 0, sizeof(procLog));
        rstrcpy( procLog.serverAddr, rodsServerHost->hostName->name, NAME_LEN );
        addProcToProcStatOut( &procLog, *procStatOut );
    }
    return status;
}

int
initProcStatOut( genQueryOut_t **procStatOut, int numProc ) {
    genQueryOut_t *myProcStatOut;

    if ( procStatOut == NULL || numProc <= 0 ) {
        return USER__NULL_INPUT_ERR;
    }

    myProcStatOut = *procStatOut = ( genQueryOut_t* )malloc( sizeof( genQueryOut_t ) );
    std::memset(myProcStatOut, 0, sizeof(genQueryOut_t));

    myProcStatOut->continueInx = -1;

    myProcStatOut->attriCnt = 9;

    myProcStatOut->sqlResult[0].attriInx = PID_INX;
    myProcStatOut->sqlResult[0].len = NAME_LEN;
    myProcStatOut->sqlResult[0].value = ( char* )malloc( NAME_LEN * numProc );
    std::memset(myProcStatOut->sqlResult[0].value, 0, NAME_LEN * numProc);

    myProcStatOut->sqlResult[1].attriInx = STARTTIME_INX;
    myProcStatOut->sqlResult[1].len = NAME_LEN;
    myProcStatOut->sqlResult[1].value = ( char* )malloc( NAME_LEN * numProc );
    std::memset(myProcStatOut->sqlResult[1].value, 0, NAME_LEN * numProc);

    myProcStatOut->sqlResult[2].attriInx = CLIENT_NAME_INX;
    myProcStatOut->sqlResult[2].len = NAME_LEN;
    myProcStatOut->sqlResult[2].value = ( char* )malloc( NAME_LEN * numProc );
    std::memset(myProcStatOut->sqlResult[2].value, 0, NAME_LEN * numProc);

    myProcStatOut->sqlResult[3].attriInx = CLIENT_ZONE_INX;
    myProcStatOut->sqlResult[3].len = NAME_LEN;
    myProcStatOut->sqlResult[3].value = ( char* )malloc( NAME_LEN * numProc );
    std::memset(myProcStatOut->sqlResult[3].value, 0, NAME_LEN * numProc);

    myProcStatOut->sqlResult[4].attriInx = PROXY_NAME_INX;
    myProcStatOut->sqlResult[4].len = NAME_LEN;
    myProcStatOut->sqlResult[4].value = ( char* )malloc( NAME_LEN * numProc );
    std::memset(myProcStatOut->sqlResult[4].value, 0, NAME_LEN * numProc);

    myProcStatOut->sqlResult[5].attriInx = PROXY_ZONE_INX;
    myProcStatOut->sqlResult[5].len = NAME_LEN;
    myProcStatOut->sqlResult[5].value = ( char* )malloc( NAME_LEN * numProc );
    std::memset(myProcStatOut->sqlResult[5].value, 0, NAME_LEN * numProc);

    myProcStatOut->sqlResult[6].attriInx = REMOTE_ADDR_INX;
    myProcStatOut->sqlResult[6].len = NAME_LEN;
    myProcStatOut->sqlResult[6].value = ( char* )malloc( NAME_LEN * numProc );
    std::memset( myProcStatOut->sqlResult[6].value, 0, NAME_LEN * numProc);

    myProcStatOut->sqlResult[7].attriInx = SERVER_ADDR_INX;
    myProcStatOut->sqlResult[7].len = NAME_LEN;
    myProcStatOut->sqlResult[7].value = ( char* )malloc( NAME_LEN * numProc );
    std::memset( myProcStatOut->sqlResult[7].value, 0, NAME_LEN * numProc);

    myProcStatOut->sqlResult[8].attriInx = PROG_NAME_INX;
    myProcStatOut->sqlResult[8].len = NAME_LEN;
    myProcStatOut->sqlResult[8].value = ( char* )malloc( NAME_LEN * numProc );
    std::memset( myProcStatOut->sqlResult[8].value, 0, NAME_LEN * numProc);

    return 0;
}

int
addProcToProcStatOut( procLog_t *procLog, genQueryOut_t *procStatOut ) {
    int rowCnt;

    if ( procLog == NULL || procStatOut == NULL ) {
        return USER__NULL_INPUT_ERR;
    }
    rowCnt = procStatOut->rowCnt;

    snprintf( &procStatOut->sqlResult[0].value[NAME_LEN * rowCnt],
              NAME_LEN, "%d", procLog->pid );
    snprintf( &procStatOut->sqlResult[1].value[NAME_LEN * rowCnt],
              NAME_LEN, "%u", procLog->startTime );
    rstrcpy( &procStatOut->sqlResult[2].value[NAME_LEN * rowCnt],
             procLog->clientName, NAME_LEN );
    rstrcpy( &procStatOut->sqlResult[3].value[NAME_LEN * rowCnt],
             procLog->clientZone, NAME_LEN );
    rstrcpy( &procStatOut->sqlResult[4].value[NAME_LEN * rowCnt],
             procLog->proxyName, NAME_LEN );
    rstrcpy( &procStatOut->sqlResult[5].value[NAME_LEN * rowCnt],
             procLog->proxyZone, NAME_LEN );
    rstrcpy( &procStatOut->sqlResult[6].value[NAME_LEN * rowCnt],
             procLog->remoteAddr, NAME_LEN );
    rstrcpy( &procStatOut->sqlResult[7].value[NAME_LEN * rowCnt],
             procLog->serverAddr, NAME_LEN );
    rstrcpy( &procStatOut->sqlResult[8].value[NAME_LEN * rowCnt],
             procLog->progName, NAME_LEN );

    procStatOut->rowCnt++;

    return 0;
}
