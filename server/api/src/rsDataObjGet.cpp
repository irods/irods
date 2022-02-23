#include "irods/dataGet.h"
#include "irods/dataObjClose.h"
#include "irods/dataObjGet.h"
#include "irods/dataObjOpen.h"
#include "irods/fileGet.h"
#include "irods/getRemoteZoneResc.h"
#include "irods/objMetaOpr.hpp"
#include "irods/physPath.hpp"
#include "irods/rcGlobalExtern.h"
#include "irods/rodsLog.h"
#include "irods/rsApiHandler.hpp"
#include "irods/rsDataGet.hpp"
#include "irods/rsDataObjRead.hpp"
#include "irods/rsDataObjClose.hpp"
#include "irods/rsDataObjGet.hpp"
#include "irods/rsDataObjOpen.hpp"
#include "irods/rsFileGet.hpp"
#include "irods/rsFileLseek.hpp"
#include "irods/rsGlobalExtern.hpp"
#include "irods/specColl.hpp"
#include "irods/subStructFileGet.h"

// =-=-=-=-=-=-=-
#include "irods/irods_resource_redirect.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/irods_resource_backport.hpp"
#include "irods/key_value_proxy.hpp"
#include "irods/replica_proxy.hpp"

namespace
{
    namespace ir = irods::experimental::replica;

    auto close_replica(RsComm& _comm, const int _fd) -> int
    {
        OpenedDataObjInp close_inp{};
        close_inp.l1descInx = _fd;
        return rsDataObjClose(&_comm, &close_inp);
    } // close_replica

    auto single_buffer_get(
        RsComm& _comm,
        const int _fd,
        portalOprOut** _out_for_checksum,
        BytesBuf* _bbuf) -> int
    {
        const auto replica = ir::make_replica_proxy(*L1desc[_fd].dataObjInfo);

        std::string checksum{};

        if (replica.cond_input().contains(VERIFY_CHKSUM_KW)) {
            if (!replica.checksum().empty()) {
                checksum = replica.checksum().data();
            }
            else {
                char* checksum_str = nullptr;
                const auto free_checksum_str = irods::at_scope_exit{[checksum_str] { std::free(checksum_str); }};

                if (const auto ec = dataObjChksumAndReg(&_comm, replica.get(), &checksum_str); ec < 0) {
                    if (const auto close_ec = close_replica(_comm, _fd); close_ec < 0) {
                        irods::log(LOG_ERROR, fmt::format(
                            "[{}:{}] - failed to close replica [error_code=[{}]]",
                            __FUNCTION__, __LINE__, close_ec));
                    }
                    return ec;
                }

                checksum = checksum_str;
            }
        }

        OpenedDataObjInp read_inp{};
        read_inp.l1descInx = _fd;
        read_inp.len = _bbuf->len;

        const int bytes_read = rsDataObjRead(&_comm, &read_inp, _bbuf);

        if (bytes_read < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to read replica "
                "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, bytes_read,
                replica.logical_path(), replica.hierarchy()));
        }
        else if (0 == bytes_read) {
            // Because clients may expect a buffer to always be returned, we set the
            // length of the returned buffer to zero to indicate that it doesn't contain
            // any valid data. A better solution is to not allocate anything and the clients
            // use the bytes read as the indicator for how to move forward.
            //
            // This solution fixes this issue for clients that incorrectly use the buffer's
            // size to determine if data was read.
            _bbuf->len = 0;
        }

        L1desc[_fd].oprStatus = bytes_read;

        if (bytes_read >= 0 && !checksum.empty()) {
            rstrcpy((*_out_for_checksum)->chksum, checksum.data(), NAME_LEN);
        }

        const auto ec = close_replica(_comm, _fd);

        if (ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to close replica [error_code=[{}]]",
                __FUNCTION__, __LINE__, ec));
        }

        return bytes_read < 0 ? bytes_read : ec;
    } // single_buffer_get

    int parallel_transfer_get(
        rsComm_t *rsComm,
        dataObjInp_t *dataObjInp,
        const int _fd,
        portalOprOut_t **portalOprOut,
        BytesBuf* dataObjOutBBuf)
    {
        char *chksumStr = NULL;
        const auto free_checksum_str = irods::at_scope_exit{[chksumStr] { std::free(chksumStr); }};

        dataObjInfo_t* dataObjInfo = L1desc[_fd].dataObjInfo;
        copyKeyVal( &dataObjInp->condInput, &dataObjInfo->condInput );

        if ( getStructFileType( dataObjInfo->specColl ) >= 0) {
            /* l3descInx == 0 if included */
            ( *portalOprOut )->l1descInx = _fd;
            return _fd;
        }

        if ( getValByKey( &dataObjInp->condInput, VERIFY_CHKSUM_KW ) != NULL ) {
            if ( strlen( dataObjInfo->chksum ) > 0 ) {
                /* a chksum already exists */
                chksumStr = strdup( dataObjInfo->chksum );
            }
            else {
                int status = dataObjChksumAndReg( rsComm, dataObjInfo, &chksumStr );
                if ( status < 0 ) {
                    if (const auto ec = close_replica(*rsComm, _fd); ec < 0) {
                        irods::log(LOG_ERROR, fmt::format(
                            "[{}:{}] - failed to close replica [error_code=[{}]]",
                            __FUNCTION__, __LINE__, ec));
                    }

                    return status;
                }
                rstrcpy( dataObjInfo->chksum, chksumStr, NAME_LEN );
            }
        }

        int status = preProcParaGet( rsComm, _fd, portalOprOut );

        if ( status < 0 ) {
            openedDataObjInp_t dataObjCloseInp{};
            dataObjCloseInp.l1descInx = _fd;
            rsDataObjClose( rsComm, &dataObjCloseInp );
            return status;
        }

        status = _fd;         /* means file not included */
        if ( chksumStr != NULL ) {
            rstrcpy( ( *portalOprOut )->chksum, chksumStr, NAME_LEN );
        }

        /* return portalOprOut to the client and wait for the rcOprComplete
         * call. That is when the parallel I/O is done */
        int retval = sendAndRecvBranchMsg( rsComm, rsComm->apiInx, status,
                                       ( void * ) * portalOprOut, dataObjOutBBuf );

        if ( retval < 0 ) {
            openedDataObjInp_t dataObjCloseInp{};
            dataObjCloseInp.l1descInx = _fd;
            rsDataObjClose( rsComm, &dataObjCloseInp );
        }

        /* already send the client the status */
        return SYS_NO_HANDLER_REPLY_MSG;
    } // parallel_transfer_get
} // anonymous namespace

int rsDataObjGet(
    rsComm_t *rsComm,
    dataObjInp_t *dataObjInp,
    portalOprOut_t **portalOprOut,
    bytesBuf_t *dataObjOutBBuf)
{
    if (!dataObjOutBBuf) {
        rodsLog(LOG_ERROR, "dataObjOutBBuf was null in call to rsDataObjGet.");
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    remove_trailing_path_separators(dataObjInp->objPath);

    auto cond_input = irods::experimental::make_key_value_proxy(dataObjInp->condInput);

    // -R and -r are not compatible
    if (cond_input.contains(RESC_NAME_KW) && cond_input.contains(RECURSIVE_OPR__KW)) {
        return USER_INCOMPATIBLE_PARAMS;
    }

    specCollCache_t *specCollCache = NULL;
    resolveLinkedPath( rsComm, dataObjInp->objPath, &specCollCache, cond_input.get());

    rodsServerHost_t *rodsServerHost;
    int remoteFlag = getAndConnRemoteZone( rsComm, dataObjInp, &rodsServerHost, REMOTE_OPEN);

    if (remoteFlag < 0) {
        return remoteFlag;
    }
    else if (LOCAL_HOST != remoteFlag) {
        int status = _rcDataObjGet(rodsServerHost->conn, dataObjInp, portalOprOut, dataObjOutBBuf);

        if (status < 0) {
            return status;
        }

        if ( status == 0 || dataObjOutBBuf->len > 0 ) {
            /* data included in buf */
            return status;
        }
        else if ( !( *portalOprOut ) ) {
            rodsLog( LOG_ERROR, "_rcDataObjGet returned a %d status code, but left portalOprOut null.", status );
            return SYS_INVALID_PORTAL_OPR;
        }
        else {
            /* have to allocate a local l1descInx to keep track of things
             * since the file is in remote zone. It sets remoteL1descInx,
             * oprType = REMOTE_ZONE_OPR and remoteZoneHost so that
             * rsComplete knows what to do */
            int l1descInx = allocAndSetL1descForZoneOpr(
                            ( *portalOprOut )->l1descInx, dataObjInp, rodsServerHost, NULL );
            if ( l1descInx < 0 ) {
                return l1descInx;
            }
            ( *portalOprOut )->l1descInx = l1descInx;
            return status;
        }
    }

    // =-=-=-=-=-=-=-
    // working on the "home zone", determine if we need to redirect to a different
    // server in this zone for this operation.  if there is a RESC_HIER_STR_KW then
    // we know that the redirection decision has already been made
    if (!cond_input.contains(RESC_HIER_STR_KW)) {
        try {
            auto result = irods::resolve_resource_hierarchy(irods::OPEN_OPERATION, rsComm, *dataObjInp);
            cond_input[RESC_HIER_STR_KW] = std::get<std::string>(result);
        }
        catch (const irods::exception& e) {
            irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.client_display_what()));
            return e.code();
        }
    }

    // Only accept the resolved hierarchy if it descends from the resource specified by the user.
    const auto hier = irods::hierarchy_parser{cond_input.at(RESC_HIER_STR_KW).value().data()};
    if (irods::experimental::keyword_has_a_value(*cond_input.get(), RESC_NAME_KW) &&
        !hier.contains(cond_input.at(RESC_NAME_KW).value())) {
        const auto msg = fmt::format(
            "hierarchy descending from specified resource name does not "
            "have a replica or the replica is inaccessible at this time "
            "[path=[{}], resource name=[{}], resolved hierarchy=[{}]]",
            dataObjInp->objPath, cond_input.at(RESC_NAME_KW).value(), hier.str());

        addRErrorMsg(&rsComm->rError, SYS_REPLICA_INACCESSIBLE, msg.data());

        irods::log(LOG_WARNING, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, msg));

        return SYS_REPLICA_INACCESSIBLE;
    }

    dataObjInp->openFlags = O_RDONLY;

    const int fd = rsDataObjOpen(rsComm, dataObjInp);
    if (fd < 0) {
        return fd;
    }

    L1desc[fd].oprType = GET_OPR;

    *portalOprOut = static_cast<portalOprOut_t*>(malloc(sizeof(portalOprOut_t)));
    std::memset(*portalOprOut, 0, sizeof(portalOprOut_t));

    try {
        const auto replica_size = L1desc[fd].dataObjInfo->dataSize;
        if (const auto single_buffer_size = irods::get_advanced_setting<const int>(irods::CFG_MAX_SIZE_FOR_SINGLE_BUFFER) * 1024 * 1024;
            replica_size <= single_buffer_size && UNKNOWN_FILE_SZ != replica_size)
        {
            dataObjOutBBuf->buf = std::malloc(single_buffer_size);
            std::memset(dataObjOutBBuf, 0, sizeof(single_buffer_size));
            dataObjOutBBuf->len = single_buffer_size;

            return single_buffer_get(*rsComm, fd, portalOprOut, dataObjOutBBuf);
        }
    }
    catch (const irods::exception& e) {
        // Any irods::exception thrown here is likely from single buffer max size not being
        // set properly. In this case, close the opened data object and return error.
        irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.client_display_what()));

        if (const auto ec = close_replica(*rsComm, fd); ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to close replica [error_code=[{}]]",
                __FUNCTION__, __LINE__, ec));
        }

        return e.code();
    }

    return parallel_transfer_get(rsComm, dataObjInp, fd, portalOprOut, dataObjOutBBuf);
} // rsDataObjGet


/* preProcParaGet - preprocessing for parallel get. Basically it calls
 * rsDataGet to setup portalOprOut with the resource server.
 */
int
preProcParaGet( rsComm_t *rsComm, int l1descInx, portalOprOut_t **portalOprOut ) {
    int status;
    dataOprInp_t dataOprInp;

    initDataOprInp( &dataOprInp, l1descInx, GET_OPR );
    /* add RESC_HIER_STR_KW for getNumThreads */
    if ( L1desc[l1descInx].dataObjInfo != NULL ) {
        //addKeyVal (&dataOprInp.condInput, RESC_NAME_KW,
        //           L1desc[l1descInx].dataObjInfo->rescInfo->rescName);
        addKeyVal( &dataOprInp.condInput, RESC_HIER_STR_KW,
                   L1desc[l1descInx].dataObjInfo->rescHier );
    }
    if ( L1desc[l1descInx].remoteZoneHost != NULL ) {
        status =  remoteDataGet( rsComm, &dataOprInp, portalOprOut,
                                 L1desc[l1descInx].remoteZoneHost );
    }
    else {
        status =  rsDataGet( rsComm, &dataOprInp, portalOprOut );
    }

    if ( status >= 0 ) {
        ( *portalOprOut )->l1descInx = l1descInx;
    }
    clearKeyVal( &dataOprInp.condInput );
    return status;
}
