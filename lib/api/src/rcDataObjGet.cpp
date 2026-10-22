#include "irods/dataObjGet.h"

#include "irods/procApiRequest.h"
#include "irods/apiNumber.h"
#include "irods/rcPortalOpr.h"
#include "irods/apiHeaderAll.h"
#include "irods/sockComm.h"
#include "irods/irods_client_server_negotiation.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/checksum.h"
#include "irods/client_connection.hpp"
#include "irods/get_file_descriptor_info.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/replica_close.h"
#include "irods/replica_open.h"
#include "irods/ticketAdmin.h"
#include "irods/dataObjLseek.h"
#include "irods/dataObjRead.h"
#include "irods/rcGlobalExtern.h"
#include "irods/rodsErrorTable.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace
{
    auto read_segment(const rodsEnv& env, const char* host, int port,
                      const char* object, int replica_number, const char* hierarchy,
                      const char* ticket, int local_fd, rodsLong_t begin, rodsLong_t end,
                      RcComm* primary, std::atomic<rodsLong_t>& progress,
                      std::mutex& progress_mutex) -> int
    {
        try {
            irods::experimental::client_connection worker{
                host, port, {env.rodsUserName, env.rodsZone}};
            auto* comm = static_cast<RcComm*>(worker);

            if (ticket) {
                ticketAdminInp_t t{};
                t.arg1 = const_cast<char*>("session");
                t.arg2 = const_cast<char*>(ticket);
                t.arg3 = t.arg4 = t.arg5 = t.arg6 = const_cast<char*>("");
                if (const int ec = rcTicketAdmin(comm, &t); ec < 0) {
                    return ec;
                }
            }

            dataObjInp_t open_input{};
            const irods::at_scope_exit clear_input{[&] { clearKeyVal(&open_input.condInput); }};
            rstrcpy(open_input.objPath, object, MAX_NAME_LEN);
            open_input.openFlags = O_RDONLY;
            const auto repl = std::to_string(replica_number);
            addKeyVal(&open_input.condInput, REPL_NUM_KW, repl.c_str());
            addKeyVal(&open_input.condInput, RESC_HIER_STR_KW, hierarchy);
            if (ticket) {
                addKeyVal(&open_input.condInput, TICKET_KW, ticket);
            }

            char* info{};
            const int fd = rc_replica_open(comm, &open_input, &info);
            std::free(info);
            if (fd < 3) {
                return fd < 0 ? fd : SYS_FILE_DESC_OUT_OF_RANGE;
            }

            const auto close_input = nlohmann::json{{"fd", fd}}.dump();
            bool replica_closed = false;
            const irods::at_scope_exit close_replica{[&] {
                if (!replica_closed) {
                    rc_replica_close(comm, close_input.c_str());
                }
            }};

            openedDataObjInp_t seek{};
            seek.l1descInx = fd;
            seek.offset = begin;
            seek.whence = SEEK_SET;
            fileLseekOut_t* seek_result{};
            const int seek_ec = rcDataObjLseek(comm, &seek, &seek_result);
            const bool at_start = seek_result && seek_result->offset == begin;
            std::free(seek_result);
            if (seek_ec < 0) {
                return seek_ec;
            }
            if (!at_start) {
                return UNIX_FILE_LSEEK_ERR;
            }

            const int chunk_size = std::max(1, env.irodsTransBufferSizeForParaTrans) * 1024 * 1024;
            for (auto pos = begin; pos < end;) {
                openedDataObjInp_t read{};
                read.l1descInx = fd;
                read.len = static_cast<int>(std::min<rodsLong_t>(end - pos, chunk_size));
                bytesBuf_t bytes{};
                const int nread = rcDataObjRead(comm, &read, &bytes);
                const irods::at_scope_exit free_bytes{[&] { std::free(bytes.buf); }};
                if (nread < 0) {
                    return nread;
                }
                if (nread == 0 || !bytes.buf || nread > read.len || bytes.len != nread) {
                    return SYS_COPY_LEN_ERR;
                }

                for (int written = 0; written < nread;) {
                    const auto n = pwrite(local_fd, static_cast<char*>(bytes.buf) + written,
                                          nread - written, pos + written);
                    if (n < 0 && errno == EINTR) {
                        continue;
                    }
                    if (n <= 0) {
                        return n < 0 ? UNIX_FILE_WRITE_ERR - errno : SYS_COPY_LEN_ERR;
                    }
                    written += n;
                }
                pos += nread;

                if (gGuiProgressCB) {
                    const auto total = progress.fetch_add(nread) + nread;
                    const std::scoped_lock lock{progress_mutex};
                    primary->operProgress.curFileSizeDone = std::max(primary->operProgress.curFileSizeDone, total);
                    gGuiProgressCB(&primary->operProgress);
                }
            }

            const int close_ec = rc_replica_close(comm, close_input.c_str());
            if (close_ec == 0) {
                replica_closed = true;
            }
            return close_ec;
        }
        catch (const irods::exception& e) {
            return e.code();
        }
        catch (const std::exception&) {
            return SYS_INTERNAL_ERR;
        }
    }

    auto get_file_multi_connection(RcComm* conn, DataObjInp* input, char* local_path,
                                   int fd, int thread_count, const char* ticket) -> int
    {
        if (thread_count < 1 || thread_count > MAX_NUM_CONFIG_TRAN_THR) {
            return SYS_INVALID_INPUT_PARAM;
        }

        try {
            const auto request = nlohmann::json{{"fd", fd}}.dump();
            char* descriptor_json{};
            const int ec = rc_get_file_descriptor_info(conn, request.c_str(), &descriptor_json);
            const irods::at_scope_exit free_info{[&] { std::free(descriptor_json); }};
            if (ec != 0 || !descriptor_json) {
                return ec != 0 ? ec : SYS_INTERNAL_ERR;
            }

            const auto info = nlohmann::json::parse(descriptor_json).at("data_object_info");
            const auto replica_number = info.at("replica_number").get<int>();
            const auto replica_size = info.at("data_size").get<rodsLong_t>();
            const auto hierarchy = info.at("resource_hierarchy").get<std::string>();

            // Stale or special-collection replicas may not have a reliable
            // catalog size. Read those until EOF on the primary connection.
            if (!info.at("special_collection").is_null() ||
                info.at("replica_status").get<int>() != GOOD_REPLICA || replica_size <= 0)
            {
                return getFile(conn, fd, local_path, input->objPath, input->dataSize);
            }
            if (input->dataSize > 0 && replica_size != input->dataSize) {
                return SYS_COPY_LEN_ERR;
            }

            rodsEnv env{};
            if (const int env_ec = getRodsEnv(&env); env_ec != 0) {
                return env_ec;
            }

            const int local_fd = open(local_path, O_WRONLY | O_CREAT | O_TRUNC, 0640);
            if (local_fd < 0) {
                return UNIX_FILE_OPEN_ERR - errno;
            }
            bool file_closed = false;
            const irods::at_scope_exit close_file{[&] {
                if (!file_closed) {
                    close(local_fd);
                }
            }};

            if (gGuiProgressCB) {
                conn->operProgress.flag = 1;
            }

            const int workers = std::min<rodsLong_t>(thread_count, replica_size);
            std::vector<int> results(workers, 0);
            std::vector<std::jthread> threads;
            std::atomic<rodsLong_t> progress{0};
            std::mutex progress_mutex;
            threads.reserve(workers);
            for (int i = 0; i < workers; ++i) {
                const auto begin = (replica_size / workers) * i;
                const auto end = i == workers - 1 ? replica_size : (replica_size / workers) * (i + 1);
                threads.emplace_back([&, i, begin, end] {
                    results[i] = read_segment(env, conn->host, conn->portNum,
                                              input->objPath, replica_number, hierarchy.c_str(),
                                              ticket, local_fd, begin, end,
                                              conn, progress, progress_mutex);
                });
            }
            threads.clear(); // All workers finish before completing the GET operation.

            for (const auto result : results) {
                if (result != 0) {
                    return result;
                }
            }
            struct stat local_stat{};
            if (fstat(local_fd, &local_stat) != 0 || local_stat.st_size != replica_size) {
                return SYS_COPY_LEN_ERR;
            }
            if (close(local_fd) != 0) {
                file_closed = true;
                return UNIX_FILE_CLOSE_ERR - errno;
            }
            file_closed = true;
            conn->transStat.bytesWritten = replica_size;
            conn->transStat.numThreads = workers;
            if (gGuiProgressCB) {
                conn->operProgress.curFileSizeDone = replica_size;
                gGuiProgressCB(&conn->operProgress);
            }
            if (getValByKey(&input->condInput, VERY_VERBOSE_KW)) {
                printf("Using multi-1247 transfer with %d connections to %s:%d\n",
                       workers, conn->host, conn->portNum);
            }
            return 0;
        }
        catch (const irods::exception& e) {
            return e.code();
        }
        catch (const std::exception&) {
            return SYS_INTERNAL_ERR;
        }
    }
} // anonymous namespace

/**
 * \fn rcDataObjGet (rcComm_t *conn, dataObjInp_t *dataObjInp,
 *   char *locFilePath)
 *
 * \brief Get (download) a data object from iRODS.
 *
 * \user client
 *
 * \ingroup data_object
 *
 * \since 1.0
 *
 *
 * \remark none
 *
 * \note none
 *
 * \usage
 * Get (download) a data object /myZone/home/john/myfile:
 * \n dataObjInp_t dataObjInp;
 * \n char locFilePath[MAX_NAME_LEN];
 * \n memset(&dataObjInp, 0, sizeof(dataObjInp));
 * \n rstrcpy (dataObjInp.objPath, "/myZone/home/john/myfile", MAX_NAME_LEN);
 * \n rstrcpy (locFilePath, "./mylocalfile", MAX_NAME_LEN);
 * \n dataObjInp.dataSize = 0;
 * \n status = rcDataObjGet (conn, &dataObjInp, locFilePath);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjInp - Elements of dataObjInp_t used :
 *    \li char \b objPath[MAX_NAME_LEN] - full path of the data object.
 *    \li rodsLong_t \b dataSize - the size of the data object.
 *      Input 0 if not known.
 *    \li int \b numThreads - the number of threads to use. Valid values are:
 *      \n NO_THREADING (-1) - no multi-thread
 *      \n 0 - the server will decide the number of threads.
 *        (recommended setting).
 *      \n A positive integer - specifies the number of threads.
 *    \li keyValPair_t \b condInput - keyword/value pair input. Valid keywords:
 *    \n RESC_NAME_KW - The resource of the data object to open.
 *    \n REPL_NUM_KW - the replica number of the copy to open.
 *    \n FORCE_FLAG_KW - overwrite existing local copy. This keyWd has no value.
 *    \n VERIFY_CHKSUM_KW - verify the checksum value of the local file after
 *           the download. This keyWd has no value.
 * \param[in] locFilePath - the path of the local file to download. This path
 *           can be a relative path.
 *
 * \return integer
 * \retval 0 on success

 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 **/

static int
rcDataObjGetImpl( rcComm_t *conn, dataObjInp_t *dataObjInp, char *locFilePath,
                  int worker_count, const char* ticket ) {
#ifndef windows_platform
    struct stat statbuf;
#else
    struct irodsntstat statbuf;
#endif

    if ( strcmp( locFilePath, STDOUT_FILE_NAME ) == 0 ) {
        /* no parallel I/O if pipe to stdout */
        dataObjInp->numThreads = NO_THREADING;
    }
#ifndef windows_platform
    else if ( stat( locFilePath, &statbuf ) >= 0 )
#else
    else if ( iRODSNt_stat( locFilePath, &statbuf ) >= 0 )
#endif
    {
        /* local file exists */
        if ( getValByKey( &dataObjInp->condInput, FORCE_FLAG_KW ) == NULL ) {
            return OVERWRITE_WITHOUT_FORCE_FLAG;
        }
    }

    portalOprOut_t *portalOprOut = NULL;
    bytesBuf_t dataObjOutBBuf{};
    const int requested_threads = dataObjInp->numThreads;
    if (worker_count > 0) {
        // Keep replica selection and checksum preparation on DATA_OBJ_GET,
        // but do not start a transfer portal on the server.
        dataObjInp->numThreads = NO_THREADING;
    }
    int status = _rcDataObjGet( conn, dataObjInp, &portalOprOut, &dataObjOutBBuf );
    dataObjInp->numThreads = requested_threads;

    if ( status < 0 ) {
        free( portalOprOut );
        return status;
    }

    if ( status == 0 || dataObjOutBBuf.len > 0 ) {
        /* data included */
        status = getIncludeFile( conn, &dataObjOutBBuf, locFilePath );
        free( dataObjOutBBuf.buf );
    }
    else if ( !portalOprOut ) {
        rodsLog( LOG_ERROR, "_rcDataObjGet returned a %d status code, but left portalOprOut null.", status );
        return SYS_INVALID_PORTAL_OPR;
    }
    else {

        if ( portalOprOut->numThreads <= 0 ) {
            if (worker_count > 0) {
                status = get_file_multi_connection(conn, dataObjInp, locFilePath,
                                                   portalOprOut->l1descInx, worker_count, ticket);
            }
            else {
                status = getFile( conn, portalOprOut->l1descInx,
                                  locFilePath, dataObjInp->objPath, dataObjInp->dataSize );
            }
        }
        else {
            if ( getValByKey( &dataObjInp->condInput, VERY_VERBOSE_KW ) != NULL ) {
                printf( "From server: NumThreads=%d, addr:%s, port:%d, cookie=%d\n",
                        portalOprOut->numThreads, portalOprOut->portList.hostAddr,
                        portalOprOut->portList.portNum, portalOprOut->portList.cookie );
            }

            /* some sanity check */
            rodsEnv env;
            getRodsEnv( &env );
            if ( portalOprOut->numThreads >= 20 * env.irodsDefaultNumberTransferThreads ) {
                rcOprComplete( conn, SYS_INVALID_PORTAL_OPR );
                free( portalOprOut );
                return SYS_INVALID_PORTAL_OPR;
            }

            conn->transStat.numThreads = portalOprOut->numThreads;
            status = getFileFromPortal( conn, portalOprOut, locFilePath,
                                        dataObjInp->objPath, dataObjInp->dataSize );
        }
        /* just send a complete msg */
        if ( status < 0 ) {
            rcOprComplete( conn, status );
        }
        else {
            status = rcOprComplete( conn, portalOprOut->l1descInx );
        }
    }

    if ( status >= 0 && conn->fileRestart.info.numSeg > 0 ) { /* file restart */
        clearLfRestartFile( &conn->fileRestart );
    }

    if ( status >= 0 && getValByKey( &dataObjInp->condInput, VERIFY_CHKSUM_KW ) != NULL ) {
        if ( portalOprOut == NULL || strlen( portalOprOut->chksum ) == 0 ) {
            rodsLog( LOG_ERROR,
                     "rcDataObjGet: VERIFY_CHKSUM_KW set but no checksum from server" );
        }
        else {

            status = verifyChksumLocFile( locFilePath, portalOprOut->chksum, NULL );
            if ( status == USER_CHKSUM_MISMATCH ) {
                rodsLogError( LOG_ERROR, status,
                              "rcDataObjGet: checksum mismatch error for %s, status = %d",
                              locFilePath, status );
                if ( portalOprOut != NULL ) {
                    free( portalOprOut );
                }
                return status;
            }
            else if ( status < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "rcDataObjGet: chksumLocFile error for %s, status = %d",
                              locFilePath, status );
                if ( portalOprOut != NULL ) {
                    free( portalOprOut );
                }
                return status;
            }

        }
    }
    free( portalOprOut );

    return status;
}

int
rcDataObjGet( rcComm_t *conn, dataObjInp_t *dataObjInp, char *locFilePath ) {
    return rcDataObjGetImpl(conn, dataObjInp, locFilePath, 0, nullptr);
}

int rcDataObjGetMultiConnection(rcComm_t* conn, dataObjInp_t* input,
                                char* local_path, int thread_count, const char* ticket)
{
    if (!conn || !input || !local_path || thread_count < 1 ||
        thread_count > MAX_NUM_CONFIG_TRAN_THR) {
        return SYS_INVALID_INPUT_PARAM;
    }
    if (strcmp(local_path, STDOUT_FILE_NAME) == 0 ||
        conn->fileRestart.flags == FILE_RESTART_ON) {
        return rcDataObjGet(conn, input, local_path);
    }
    return rcDataObjGetImpl(conn, input, local_path, thread_count, ticket);
}

int
_rcDataObjGet( rcComm_t *conn, dataObjInp_t *dataObjInp,
               portalOprOut_t **portalOprOut, bytesBuf_t *dataObjOutBBuf ) {
    int status;

    *portalOprOut = NULL;

    memset( &conn->transStat, 0, sizeof( transStat_t ) );

    memset( dataObjOutBBuf, 0, sizeof( bytesBuf_t ) );

    dataObjInp->oprType = GET_OPR;

    status = procApiRequest( conn, DATA_OBJ_GET_AN,  dataObjInp, NULL,
                             ( void ** ) portalOprOut, dataObjOutBBuf );

    if ( *portalOprOut != NULL && ( *portalOprOut )->l1descInx < 0 ) {
        status = ( *portalOprOut )->l1descInx;
    }

    return status;
}
