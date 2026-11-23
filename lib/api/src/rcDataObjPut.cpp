#include "irods/dataObjPut.h"
#include "irods/procApiRequest.h"
#include "irods/apiNumber.h"
#include "irods/rcPortalOpr.h"
#include "irods/oprComplete.h"
#include "irods/sockComm.h"
#include "irods/rcMisc.h"
#include "irods/irods_client_server_negotiation.hpp"
#include "irods/client_connection.hpp"
#include "irods/get_file_descriptor_info.h"
#include "irods/replica_open.h"
#include "irods/replica_close.h"
#include "irods/ticketAdmin.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/dataObjLseek.h"
#include "irods/dataObjWrite.h"
#include "irods/rcGlobalExtern.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

namespace
{
    // This is deliberately only a byte transport. DATA_OBJ_PUT still owns
    // resource selection, put policy, checksum verification and finalization.
    auto write_segment(const rodsEnv& env, const char* host, int port,
                       const char* path, const char* object,
                       const std::string& token, int replica_number,
                       const char* ticket, rodsLong_t offset, rodsLong_t end,
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
            open_input.openFlags = O_WRONLY; // Never truncate the primary stream's replica.
            addKeyVal(&open_input.condInput, REPLICA_TOKEN_KW, token.c_str());
            const auto repl = std::to_string(replica_number);
            addKeyVal(&open_input.condInput, REPL_NUM_KW, repl.c_str());

            char* info{};
            const int fd = rc_replica_open(comm, &open_input, &info);
            std::free(info);
            if (fd < 3) {
                return fd < 0 ? fd : SYS_FILE_DESC_OUT_OF_RANGE;
            }

            const auto close_input = nlohmann::json{{"fd", fd}, {"update_size", false},
                                                    {"update_status", false}, {"compute_checksum", false},
                                                    {"send_notifications", false}}.dump();
            bool replica_closed = false;
            const irods::at_scope_exit close_replica{[&] {
                if (!replica_closed) {
                    rc_replica_close(comm, close_input.c_str());
                }
            }};

            openedDataObjInp_t seek{};
            seek.l1descInx = fd;
            seek.offset = offset;
            seek.whence = SEEK_SET;
            fileLseekOut_t* seek_result{};
            const int seek_ec = rcDataObjLseek(comm, &seek, &seek_result);
            const bool at_requested_offset = seek_result && seek_result->offset == offset;
            std::free(seek_result);
            if (seek_ec < 0) {
                return seek_ec;
            }
            if (!at_requested_offset) {
                return UNIX_FILE_LSEEK_ERR;
            }

            const int local_fd = open(path, O_RDONLY);
            if (local_fd < 0) {
                return UNIX_FILE_OPEN_ERR - errno;
            }
            const irods::at_scope_exit close_file{[&] { close(local_fd); }};

            const auto chunk_size = std::max(1, env.irodsTransBufferSizeForParaTrans) * 1024 * 1024;
            std::vector<char> buffer(chunk_size);
            for (auto pos = offset; pos < end;) {
                const auto count = static_cast<size_t>(std::min<rodsLong_t>(end - pos, buffer.size()));
                const auto nread = pread(local_fd, buffer.data(), count, pos);
                if (nread < 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    return UNIX_FILE_READ_ERR - errno;
                }
                if (nread == 0) {
                    return SYS_COPY_LEN_ERR;
                }
                openedDataObjInp_t write{};
                write.l1descInx = fd;
                write.len = nread;
                bytesBuf_t bytes{};
                bytes.buf = buffer.data();
                bytes.len = nread;
                const int written = rcDataObjWrite(comm, &write, &bytes);
                if (written != nread) {
                    return written < 0 ? written : SYS_COPY_LEN_ERR;
                }
                pos += written;
                if (gGuiProgressCB) {
                    const auto total = progress.fetch_add(written) + written;
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
} // anonymous namespace

int rcDataObjPutMultiConnection(rcComm_t* conn, dataObjInp_t* input,
                                const char* local_path, int thread_count, const char* ticket)
{
    if (!conn || !input || !local_path || thread_count < 1 ||
        thread_count > MAX_NUM_CONFIG_TRAN_THR || input->dataSize <= 0) {
        return SYS_INVALID_INPUT_PARAM;
    }

    // Do not create a portal. Retain the DATA_OBJ_PUT handshake and its
    // original finalization (including -K, -k, -a and atomic metadata/ACLs).
    std::memset(&conn->transStat, 0, sizeof(conn->transStat));
    const auto requested_threads = input->numThreads;
    input->numThreads = NO_THREADING;
    input->oprType = PUT_OPR;
    addKeyVal(&input->condInput, DATA_SIZE_KW, std::to_string(input->dataSize).c_str());
    bytesBuf_t empty{};
    portalOprOut_t* output{};
    const int open_ec = _rcDataObjPut(conn, input, &empty, &output);
    input->numThreads = requested_threads;
    if (open_ec < 0) {
        std::free(output);
        return open_ec;
    }
    if (!output) {
        return SYS_INVALID_PORTAL_OPR;
    }

    int status = 0;
    if (output->numThreads != 0 || output->l1descInx < 3) {
        status = SYS_INVALID_PORTAL_OPR;
    }
    else {
        try {
            const auto request = nlohmann::json{{"fd", output->l1descInx}}.dump();
            char* descriptor_json{};
            const int ec = rc_get_file_descriptor_info(conn, request.c_str(), &descriptor_json);
            const irods::at_scope_exit free_info{[&] { std::free(descriptor_json); }};
            if (ec != 0 || !descriptor_json) {
                status = ec != 0 ? ec : SYS_INTERNAL_ERR;
            }
            else {
                const auto info = nlohmann::json::parse(descriptor_json);
                const auto token = info.at("replica_token").get<std::string>();
                const auto replica_number = info.at("data_object_info").at("replica_number").get<int>();

                rodsEnv env{};
                if (token.empty() || !info.at("data_object_info").at("special_collection").is_null()) {
                    // A special collection cannot be reopened through the token API.
                    status = putFile(conn, output->l1descInx, const_cast<char*>(local_path),
                                     input->objPath, input->dataSize);
                }
                else if ((status = getRodsEnv(&env)) == 0) {
                    const int workers = std::min<rodsLong_t>(thread_count, input->dataSize);
                    std::vector<int> results(workers, 0);
                    std::vector<std::jthread> threads;
                    std::atomic<rodsLong_t> progress{0};
                    std::mutex progress_mutex;
                    threads.reserve(workers);
                    for (int i = 0; i < workers; ++i) {
                        const auto begin = (input->dataSize / workers) * i;
                        const auto end = i == workers - 1 ? input->dataSize : (input->dataSize / workers) * (i + 1);
                        threads.emplace_back([&, i, begin, end] {
                            results[i] = write_segment(env, conn->host, conn->portNum,
                                                       local_path, input->objPath, token,
                                                       replica_number, ticket, begin, end,
                                                       conn, progress, progress_mutex);
                        });
                    }
                    threads.clear(); // jthread joins every worker before finalization.
                    for (const auto result : results) {
                        if (result != 0 && status == 0) {
                            status = result;
                        }
                    }
                    if (status == 0) {
                        conn->transStat.bytesWritten = input->dataSize;
                        conn->transStat.numThreads = workers;
                        if (getValByKey(&input->condInput, VERY_VERBOSE_KW)) {
                            printf("Using multi-1247 transfer with %d connections to %s:%d\n",
                                   workers, conn->host, conn->portNum);
                        }
                        if (gGuiProgressCB) {
                            conn->operProgress.curFileSizeDone = input->dataSize;
                            gGuiProgressCB(&conn->operProgress);
                        }
                    }
                }
            }
        }
        catch (const std::exception&) {
            status = SYS_INTERNAL_ERR;
        }
    }

    const int completion_ec = rcOprComplete(conn, status < 0 ? status : output->l1descInx);
    std::free(output);
    return status < 0 ? status : completion_ec;
}

/**
 * \fn rcDataObjPut (rcComm_t *conn, dataObjInp_t *dataObjInp,
 *   char *locFilePath)
 *
 * \brief Put (upload) a data object to iRODS.
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
 * Put (upload) a data object /myZone/home/john/myfile in myRescource:
 * \n dataObjInp_t dataObjInp;
 * \n char locFilePath[MAX_NAME_LEN];
 * \n memset(&dataObjInp, 0, sizeof(dataObjInp));
 * \n rstrcpy (dataObjInp.objPath, "/myZone/home/john/myfile", MAX_NAME_LEN);
 * \n rstrcpy (locFilePath, "./mylocalfile", MAX_NAME_LEN);
 * \n dataObjInp.createMode = 0750;
 * \n dataObjInp.dataSize = 12345;
 * \n addKeyVal (&dataObjInp.condInput, DEST_RESC_NAME_KW, "myRescource");
 * \n status = rcDataObjPut (conn, &dataObjInp, locFilePath);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjInp - Elements of dataObjInp_t used :
 *    \li char \b objPath[MAX_NAME_LEN] - full path of the data object.
 *    \li int \b createMode - the file mode of the data object.
 *    \li rodsLong_t \b dataSize - the size of the data object.
 *      Input 0 if not known.
 *    \li int \b numThreads - the number of threads to use. Valid values are:
 *      \n NO_THREADING (-1) - no multi-thread
 *      \n 0 - the server will decide the number of threads.
 *        (recommended setting).
 *      \n A positive integer - specifies the number of threads.
 *    \li keyValPair_t \b condInput - keyword/value pair input. Valid keywords:
 *    \n DATA_TYPE_KW - the data type of the data object.
 *    \n DEST_RESC_NAME_KW - The resource to store this data object
 *    \n FORCE_FLAG_KW - overwrite existing copy. This keyWd has no value
 *    \n REPL_NUM_KW - If the data object already exist, the replica number
 *            of the copy to overwrite.
 *    \n REG_CHKSUM_KW -  register the target checksum value after the copy.
 *            The value is the md5 checksum value of the local file.
 *    \n VERIFY_CHKSUM_KW - verify and register the target checksum value
 *            after the copy. The value is the md5 checksum value of the
 *            local file.
 * \param[in] locFilePath - the path of the local file to upload. This path
 *           can be a relative path.
 *
 * \return integer
 * \retval 0 on success

 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 **/

int
rcDataObjPut( rcComm_t *conn, dataObjInp_t *dataObjInp, char *locFilePath ) {

    int status;
    portalOprOut_t *portalOprOut = NULL;
    bytesBuf_t dataObjInpBBuf;

    if ( dataObjInp->dataSize <= 0 ) {
        dataObjInp->dataSize = getFileSize( locFilePath );
        if ( dataObjInp->dataSize < 0 ) {
            return USER_FILE_DOES_NOT_EXIST;
        }
    }

    addKeyVal( &dataObjInp->condInput, DATA_SIZE_KW, std::to_string(dataObjInp->dataSize).c_str() );

    memset( &conn->transStat, 0, sizeof( transStat_t ) );
    memset( &dataObjInpBBuf, 0, sizeof( dataObjInpBBuf ) );

    rodsEnv rods_env;
    if ( int status = getRodsEnv( &rods_env ) ) {
        rodsLog( LOG_ERROR, "getRodsEnv failed in %s with status %s", __FUNCTION__, status );
        return status;
    }
    int single_buff_sz = rods_env.irodsMaxSizeForSingleBuffer * 1024 * 1024;
    if ( getValByKey( &dataObjInp->condInput, DATA_INCLUDED_KW ) != NULL ) {
        if ( dataObjInp->dataSize > single_buff_sz ) {
            rmKeyVal( &dataObjInp->condInput, DATA_INCLUDED_KW );
        }
        else {
            status = fillBBufWithFile( conn, &dataObjInpBBuf, locFilePath,
                                       dataObjInp->dataSize );
            if ( status < 0 ) {
                rodsLog( LOG_NOTICE,
                         "rcDataObjPut: fileBBufWithFile error for %s", locFilePath );
                return status;
            }
        }
    }
    else if ( dataObjInp->dataSize <= single_buff_sz ) {
        addKeyVal( &dataObjInp->condInput, DATA_INCLUDED_KW, "" );
        status = fillBBufWithFile( conn, &dataObjInpBBuf, locFilePath,
                                   dataObjInp->dataSize );
        if ( status < 0 ) {
            rodsLog( LOG_NOTICE,
                     "rcDataObjPut: fileBBufWithFile error for %s", locFilePath );
            return status;
        }
    }

    dataObjInp->oprType = PUT_OPR;

    status = _rcDataObjPut( conn, dataObjInp, &dataObjInpBBuf, &portalOprOut );

    clearBBuf( &dataObjInpBBuf );

    if ( status < 0 ||
            getValByKey( &dataObjInp->condInput, DATA_INCLUDED_KW ) != NULL ) {
        if ( portalOprOut != NULL ) {
            free( portalOprOut );
        }
        return status;
    }

    if ( portalOprOut->numThreads <= 0 ) {
        status = putFile( conn, portalOprOut->l1descInx,
                          locFilePath, dataObjInp->objPath, dataObjInp->dataSize );
    }
    else {
        if ( getValByKey( &dataObjInp->condInput, VERY_VERBOSE_KW ) != NULL ) {
            printf( "From server: NumThreads=%d, addr:%s, port:%d, cookie=%d\n",
                    portalOprOut->numThreads, portalOprOut->portList.hostAddr,
                    portalOprOut->portList.portNum, portalOprOut->portList.cookie );
        }
        /* some sanity check */
        rodsEnv rods_env;
        if ( int status = getRodsEnv( &rods_env ) ) {
            rodsLog( LOG_ERROR, "getRodsEnv failed in %s with status %s", __FUNCTION__, status );
            return status;
        }
        if ( portalOprOut->numThreads >= 20 * rods_env.irodsDefaultNumberTransferThreads ) {
            rcOprComplete( conn, SYS_INVALID_PORTAL_OPR );
            free( portalOprOut );
            return SYS_INVALID_PORTAL_OPR;
        }

        conn->transStat.numThreads = portalOprOut->numThreads;
        status = putFileToPortal( conn, portalOprOut, locFilePath,
                                  dataObjInp->objPath, dataObjInp->dataSize );
    }

    /* just send a complete msg */
    if ( status < 0 ) {
        rcOprComplete( conn, status );
    }
    else {
        status = rcOprComplete( conn, portalOprOut->l1descInx );
    }
    free( portalOprOut );

    if ( status >= 0 && conn->fileRestart.info.numSeg > 0 ) { /* file restart */
        clearLfRestartFile( &conn->fileRestart );
    }

    return status;
}

int
_rcDataObjPut( rcComm_t *conn, dataObjInp_t *dataObjInp,
               bytesBuf_t *dataObjInpBBuf, portalOprOut_t **portalOprOut ) {
    int status;

    status = procApiRequest( conn, DATA_OBJ_PUT_AN,  dataObjInp,
                             dataObjInpBBuf, ( void ** ) portalOprOut, NULL );

    if ( *portalOprOut != NULL && ( *portalOprOut )->l1descInx < 0 ) {
        status = ( *portalOprOut )->l1descInx;
    }

    return status;
}
