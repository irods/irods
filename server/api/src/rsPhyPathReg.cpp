#include "rsPhyPathReg.hpp"

#include "fileStat.h"
#include "key_value_proxy.hpp"
#include "phyPathReg.h"
#include "rcMisc.h"
#include "rodsLog.h"
#include "icatDefines.h"
#include "objMetaOpr.hpp"
#include "dataObjOpr.hpp"
#include "collection.hpp"
#include "specColl.hpp"
#include "resource.hpp"
#include "physPath.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "miscServerFunct.hpp"
#include "apiHeaderAll.h"
#include "rsRegReplica.hpp"
#include "rsCollCreate.hpp"
#include "rsFileOpendir.hpp"
#include "rsFileReaddir.hpp"
#include "rsFileStat.hpp"
#include "rsFileMkdir.hpp"
#include "rsFileClosedir.hpp"
#include "rsRegDataObj.hpp"
#include "rsModColl.hpp"
#include "rsRegColl.hpp"
#include "rsModAccessControl.hpp"
#include "rsDataObjCreate.hpp"
#include "rsDataObjClose.hpp"
#include "rsRegColl.hpp"
#include "rsSubStructFileStat.hpp"
#include "rsSyncMountedColl.hpp"
#include "rsFileOpen.hpp"
#include "rsFileRead.hpp"
#include "rsFileClose.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_get_l1desc.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"
#include "key_value_proxy.hpp"

#define IRODS_QUERY_ENABLE_SERVER_SIDE_API
#include "irods_query.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "replica_proxy.hpp"

#include "boost/lexical_cast.hpp"
#include "fmt/format.h"

#include <iostream>

/* holds a struct that describes pathname match patterns
   to exclude from registration. Needs to be global due
   to the recursive dirPathReg() calls. */
static pathnamePatterns_t *ExcludePatterns = NULL;

namespace {

    pathnamePatterns_t* readPathnamePatternsFromFile(
        rsComm_t *rsComm,
        char *filename,
        char* resc_hier)
    {
        int status;
        rodsStat_t *stbuf;
        fileStatInp_t fileStatInp;
        bytesBuf_t fileReadBuf;
        fileOpenInp_t fileOpenInp;
        fileReadInp_t fileReadInp;
        fileCloseInp_t fileCloseInp;
        int buf_len, fd;
        pathnamePatterns_t *pp;

        if ( rsComm == NULL || filename == NULL || resc_hier == NULL ) {
            return NULL;
        }


        // =-=-=-=-=-=-=-
        // extract the host location from the resource hierarchy
        std::string location;
        irods::error ret = irods::get_loc_for_hier_string( resc_hier, location );
        if ( !ret.ok() ) {
            irods::log( PASSMSG( "readPathnamePatternsFromFile - failed in get_loc_for_hier_string", ret ) );
            return NULL;
        }

        memset( &fileStatInp, 0, sizeof( fileStatInp ) );
        rstrcpy( fileStatInp.fileName, filename, MAX_NAME_LEN );
        rstrcpy( fileStatInp.addr.hostAddr, location.c_str(), NAME_LEN );
        rstrcpy( fileStatInp.rescHier, resc_hier, MAX_NAME_LEN );
        status = rsFileStat( rsComm, &fileStatInp, &stbuf );
        if ( status != 0 ) {
            if ( status != UNIX_FILE_STAT_ERR - ENOENT ) {
                rodsLog( LOG_DEBUG, "readPathnamePatternsFromFile: can't stat %s. status = %d",
                         fileStatInp.fileName, status );
            }
            return NULL;
        }
        buf_len = stbuf->st_size;
        free( stbuf );

        memset( &fileOpenInp, 0, sizeof( fileOpenInp ) );
        rstrcpy( fileOpenInp.fileName, filename, MAX_NAME_LEN );
        rstrcpy( fileOpenInp.addr.hostAddr, location.c_str(), NAME_LEN );
        rstrcpy( fileOpenInp.resc_hier_, resc_hier, MAX_NAME_LEN );
        fileOpenInp.flags = O_RDONLY;
        fd = rsFileOpen( rsComm, &fileOpenInp );
        if ( fd < 0 ) {
            rodsLog( LOG_NOTICE,
                     "readPathnamePatternsFromFile: can't open %s for reading. status = %d",
                     fileOpenInp.fileName, fd );
            return NULL;
        }

        memset( &fileReadBuf, 0, sizeof( fileReadBuf ) );
        fileReadBuf.buf = malloc( buf_len );
        if ( fileReadBuf.buf == NULL ) {
            rodsLog( LOG_NOTICE, "readPathnamePatternsFromFile: could not malloc buffer" );
            return NULL;
        }

        memset( &fileReadInp, 0, sizeof( fileReadInp ) );
        fileReadInp.fileInx = fd;
        fileReadInp.len = buf_len;
        status = rsFileRead( rsComm, &fileReadInp, &fileReadBuf );

        memset( &fileCloseInp, 0, sizeof( fileCloseInp ) );
        fileCloseInp.fileInx = fd;
        rsFileClose( rsComm, &fileCloseInp );

        if ( status < 0 ) {
            rodsLog( LOG_NOTICE, "readPathnamePatternsFromFile: could not read %s. status = %d",
                     fileOpenInp.fileName, status );
            free( fileReadBuf.buf );
            return NULL;
        }

        pp = readPathnamePatterns( ( char* )fileReadBuf.buf, buf_len );

        return pp;
    } // readPathnamePatternsFromFile

    int remotePhyPathReg(
        rsComm_t *rsComm,
        dataObjInp_t *phyPathRegInp,
        rodsServerHost_t *rodsServerHost)
    {
        int status;

        if ( rodsServerHost == NULL ) {
            rodsLog( LOG_ERROR,
                     "remotePhyPathReg: Invalid rodsServerHost" );
            return SYS_INVALID_SERVER_HOST;
        }

        if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
            return status;
        }

        status = rcPhyPathReg( rodsServerHost->conn, phyPathRegInp );

        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "remotePhyPathReg: rcPhyPathReg failed for %s",
                     phyPathRegInp->objPath );
        }

        return status;
    } // remotePhyPathReg

    int filePathRegRepl(
        rsComm_t *rsComm,
        dataObjInp_t *phyPathRegInp,
        char *filePath,
        const char *_resc_name)
    {
        namespace fs = irods::experimental::filesystem;

        auto phy_path_reg_cond_input = irods::experimental::make_key_value_proxy(phyPathRegInp->condInput);

        // The resource hierarchy for this replica must be provided
        if (!phy_path_reg_cond_input.contains(RESC_HIER_STR_KW)) {
            rodsLog( LOG_NOTICE, "%s - RESC_HIER_STR_KW is NULL", __FUNCTION__ );
            return SYS_INVALID_INPUT_PARAM;
        }

        dataObjInfo_t* data_obj_info_head{};
        if (const int ec = getDataObjInfo(rsComm, phyPathRegInp, &data_obj_info_head, ACCESS_READ_OBJECT, 0); ec < 0) {
            rodsLog(LOG_ERROR, "%s: getDataObjInfo for [%s] failed", __FUNCTION__, phyPathRegInp->objPath);
            return ec;
        }

        // Put a desired replica at the head of the list
        if (const int ec = sortObjInfoForOpen(&data_obj_info_head, phy_path_reg_cond_input.get(), 0 ); ec < 0) {
            // we did not match the hier string but we can still continue as we have a good copy for a read
            if (!data_obj_info_head) {
                return ec;
            }
        }

        // Free the structure before exiting the function
        irods::at_scope_exit free_data_obj_info_head{ [data_obj_info_head] { freeAllDataObjInfo(data_obj_info_head); } };

        // Populate information for the replica being registered
        auto [destination_replica, destination_replica_lm] = irods::experimental::replica::make_replica_proxy();
        std::memcpy(destination_replica.get(), data_obj_info_head, sizeof(dataObjInfo_t));
        destination_replica.physical_path(filePath);
        destination_replica.resource(_resc_name);
        destination_replica.hierarchy(phy_path_reg_cond_input.at(RESC_HIER_STR_KW).value());

        // Set data_status to an empty string so that the replica status is not misleadingly labeled.
        destination_replica.status("{}");

        rodsLong_t resc_id{};
        if (const auto ret = resc_mgr.hier_to_leaf_id(destination_replica.hierarchy().data(), resc_id); !ret.ok()) {
            irods::log(PASS(ret));
        }
        destination_replica.resource_id(resc_id);

        // Prepare input for rsRegReplica
        regReplica_t reg_replica_input{};
        reg_replica_input.srcDataObjInfo = data_obj_info_head;
        reg_replica_input.destDataObjInfo = destination_replica.get();

        auto reg_replica_cond_input = irods::experimental::make_key_value_proxy(reg_replica_input.condInput);
        const auto free_cond_input = irods::at_scope_exit{[&reg_replica_input] { clearKeyVal(&reg_replica_input.condInput); }};

        // Carry privileged access keywords forward
        if (phy_path_reg_cond_input.contains(SU_CLIENT_USER_KW )) {
            reg_replica_cond_input[SU_CLIENT_USER_KW] = "";
            reg_replica_cond_input[ADMIN_KW] = "";
        }
        else if (phy_path_reg_cond_input.contains(ADMIN_KW)) {
            reg_replica_cond_input[ADMIN_KW] = "";
        }

        // Data size can be passed via DATA_SIZE_KW to save a stat later
        if (phy_path_reg_cond_input.contains(DATA_SIZE_KW)) {
            reg_replica_cond_input[DATA_SIZE_KW] = phy_path_reg_cond_input.at(DATA_SIZE_KW);
        }

        // Indicates whether data movement is expected
        if (phy_path_reg_cond_input.contains(REGISTER_AS_INTERMEDIATE_KW)) {
            destination_replica.replica_status(INTERMEDIATE_REPLICA);
            reg_replica_cond_input[REGISTER_AS_INTERMEDIATE_KW] = "";
        }

        // Registers the replica; bails on failure
        if (const int ec = rsRegReplica(rsComm, &reg_replica_input); ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}] - failed to register replica for [{}], status:[{}]",
                __FUNCTION__, destination_replica.logical_path(), ec));
            return ec;
        }

        // Free the existing opened data object info and replace with the newly created replica using opened L1 descriptor info
        if (auto* l1desc = irods::find_l1desc(fs::path{destination_replica.logical_path().data()}, destination_replica.hierarchy()); l1desc) {
            freeAllDataObjInfo(l1desc->dataObjInfo);
            l1desc->dataObjInfo = destination_replica_lm.release();
        }

        return 0;
    } // filePathRegRepl

    int filePathReg(
        rsComm_t *rsComm,
        dataObjInp_t *phyPathRegInp,
        const char *_resc_name)
    {
        namespace fs = irods::experimental::filesystem;

        auto phy_path_reg_cond_input = irods::experimental::make_key_value_proxy(phyPathRegInp->condInput);

        // The resource hierarchy for this replica must be provided
        if (!phy_path_reg_cond_input.contains(RESC_HIER_STR_KW)) {
            irods::log(LOG_NOTICE, fmt::format("[{}] - RESC_HIER_STR_KW is NULL", __FUNCTION__));
            return SYS_INVALID_INPUT_PARAM;
        }

        // Populate information for the replica being registered
        auto [destination_replica, destination_replica_lm] = irods::experimental::replica::make_replica_proxy();
        initDataObjInfoWithInp(destination_replica.get(), phyPathRegInp );
        destination_replica.replica_status(GOOD_REPLICA);
        destination_replica.resource(_resc_name);
        destination_replica.hierarchy(phy_path_reg_cond_input.at(RESC_HIER_STR_KW).value());

        rodsLong_t resc_id{};
        if (const auto ret = resc_mgr.hier_to_leaf_id(destination_replica.hierarchy().data(), resc_id); !ret.ok()) {
            irods::log(PASS(ret));
        }
        destination_replica.resource_id(resc_id);

        if (!phy_path_reg_cond_input.contains(DATA_SIZE_KW) && destination_replica.size() <= 0) {
            const auto file_size = getFileMetadataFromVault(rsComm, destination_replica.get());

            if (file_size < 0 && UNKNOWN_FILE_SZ != file_size) {
                irods::log(LOG_ERROR, fmt::format(
                     "[{}]: getFileMetadataFromVault for {} failed, status = {}",
                     __FUNCTION__, destination_replica.logical_path(), file_size));
                return file_size;
            }

            destination_replica.size(file_size);
        }
        else {
            std::string_view data_size_str = phy_path_reg_cond_input.at(DATA_SIZE_KW).value();

            try {
                destination_replica.size(boost::lexical_cast<rodsLong_t>(data_size_str));
            }
            catch (boost::bad_lexical_cast&) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}] - bad_lexical_cast for dataSize [{}]; setting to 0",
                    __FUNCTION__, data_size_str));
                destination_replica.size(0);
            }
        }

        if (phy_path_reg_cond_input.contains(DATA_MODIFY_KW)) {
            destination_replica.mtime(phy_path_reg_cond_input.at(DATA_MODIFY_KW).value());
        }

        // If intermediate, do not attempt to verify checksum as the file does not yet exist
        if (phy_path_reg_cond_input.contains(REGISTER_AS_INTERMEDIATE_KW)) {
            destination_replica.replica_status(INTERMEDIATE_REPLICA);
        }
        else if (phy_path_reg_cond_input.contains(REG_CHKSUM_KW) ||
                 phy_path_reg_cond_input.contains(VERIFY_CHKSUM_KW)) {
            char* chksum{};
            if (const int ec = _dataObjChksum(rsComm, destination_replica.get(), &chksum); ec < 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}]: _dataObjChksum for {} failed, status = {}",
                    __FUNCTION__, destination_replica.logical_path(), ec));
                return ec;
            }
            destination_replica.checksum(chksum);
        }

        // Register the data object
        const int ec = svrRegDataObj(rsComm, destination_replica.get());
        if (ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                 "{}: svrRegDataObj for {} failed, status = {}",
                 __FUNCTION__, destination_replica.logical_path(), ec));
            return ec;
        }

        // static PEP for filePathReg
        {
            ruleExecInfo_t rei;
            initReiWithDataObjInp(&rei, rsComm, phyPathRegInp);
            rei.doi = destination_replica.get();
            rei.status = ec;

            // make resource properties available as rule session variables
            irods::get_resc_properties_as_kvp(rei.doi->rescHier, rei.condInputData);

            rei.status = applyRule("acPostProcForFilePathReg", NULL, &rei, NO_SAVE_REI);

            clearKeyVal(rei.condInputData);
            free(rei.condInputData);
        } // static PEP for filePathReg

        // Free the existing opened data object info and replace with the newly created replica using opened L1 descriptor info
        if (auto* l1desc = irods::find_l1desc(fs::path{destination_replica.logical_path().data()}, destination_replica.hierarchy()); l1desc) {
            freeAllDataObjInfo(l1desc->dataObjInfo);
            l1desc->dataObjInfo = destination_replica_lm.release();
        }

        return ec;
    } // filePathReg

    int dirPathReg(
        rsComm_t *rsComm,
        dataObjInp_t *phyPathRegInp,
        char *filePath,
        const char *_resc_name)
    {
        fileStatInp_t fileStatInp;
        collInp_t collCreateInp;
        fileOpendirInp_t fileOpendirInp;
        fileClosedirInp_t fileClosedirInp;
        int status;
        int dirFd;
        dataObjInp_t subPhyPathRegInp;
        fileReaddirInp_t fileReaddirInp;
        rodsDirent_t *rodsDirent = NULL;
        rodsObjStat_t *rodsObjStatOut = NULL;
        int forceFlag;

        char* resc_hier = getValByKey( &phyPathRegInp->condInput, RESC_HIER_STR_KW );
        if ( !resc_hier ) {
            rodsLog( LOG_NOTICE, "dirPathReg - RESC_HIER_STR_KW is NULL" );
            return SYS_INVALID_INPUT_PARAM;
        }

        // =-=-=-=-=-=-=-
        // extract the host location from the resource hierarchy
        std::string location;
        irods::error ret = irods::get_loc_for_hier_string( resc_hier, location );
        if ( !ret.ok() ) {
            irods::log( PASSMSG( "dirPathReg - failed in get_loc_for_hier_string", ret ) );
            return SYS_INVALID_INPUT_PARAM;
        }

        status = collStat( rsComm, phyPathRegInp, &rodsObjStatOut );
        if ( status < 0 || NULL == rodsObjStatOut ) { // JMC cppcheck - nullptr
            freeRodsObjStat( rodsObjStatOut );
            rodsObjStatOut = NULL;
            memset( &collCreateInp, 0, sizeof( collCreateInp ) );
            rstrcpy( collCreateInp.collName, phyPathRegInp->objPath,
                     MAX_NAME_LEN );

            /* no need to resolve sym link */ // JMC - backport 4845
            addKeyVal( &collCreateInp.condInput, TRANSLATED_PATH_KW, "" ); // JMC - backport 4845

            /* stat the source directory to track the         */
            /* original directory meta-data                   */
            memset( &fileStatInp, 0, sizeof( fileStatInp ) );
            rstrcpy( fileStatInp.fileName, filePath, MAX_NAME_LEN );

            // Get resource location
            rodsLong_t resc_id = 0;
            ret = resc_mgr.hier_to_leaf_id(_resc_name,resc_id);
            if(!ret.ok()) {
                return ret.code();
            }
            std::string location;
            irods::error ret = irods::get_resource_property< std::string >( resc_id, irods::RESOURCE_LOCATION, location );
            if ( !ret.ok() ) {
                irods::log PASSMSG( "dirPathReg - failed in get_resource_property", ret );
                return ret.code();
            }

            snprintf( fileStatInp.addr.hostAddr, NAME_LEN, "%s", location.c_str() );

            rstrcpy( fileStatInp.rescHier, resc_hier, MAX_NAME_LEN );
            rstrcpy( fileStatInp.objPath, phyPathRegInp->objPath, MAX_NAME_LEN );

            /* create the coll just in case it does not exist */
            status = rsCollCreate( rsComm, &collCreateInp );
            clearKeyVal( &collCreateInp.condInput ); // JMC - backport 4835
            if ( status < 0 ) {
                return status;
            }
        }
        else if ( rodsObjStatOut->specColl != NULL ) {
            freeRodsObjStat( rodsObjStatOut );
            rodsLog( LOG_ERROR,
                     "mountFileDir: %s already mounted", phyPathRegInp->objPath );
            return SYS_MOUNT_MOUNTED_COLL_ERR;
        }
        freeRodsObjStat( rodsObjStatOut );
        rodsObjStatOut = NULL;

        memset( &fileOpendirInp, 0, sizeof( fileOpendirInp ) );

        rstrcpy( fileOpendirInp.dirName, filePath, MAX_NAME_LEN );
        rstrcpy( fileOpendirInp.addr.hostAddr, location.c_str(), NAME_LEN );
        rstrcpy( fileOpendirInp.objPath,    phyPathRegInp->objPath, MAX_NAME_LEN );
        rstrcpy( fileOpendirInp.resc_hier_, resc_hier,              MAX_NAME_LEN );

        dirFd = rsFileOpendir( rsComm, &fileOpendirInp );
        if ( dirFd < 0 ) {
            rodsLog( LOG_ERROR,
                     "dirPathReg: rsFileOpendir for %s error, status = %d",
                     filePath, dirFd );
            return dirFd;
        }

        fileReaddirInp.fileInx = dirFd;

        if ( getValByKey( &phyPathRegInp->condInput, FORCE_FLAG_KW ) != NULL ) {
            forceFlag = 1;
        }
        else {
            forceFlag = 0;
        }

        while ( ( status = rsFileReaddir( rsComm, &fileReaddirInp, &rodsDirent ) ) >= 0 ) {

            if ( NULL == rodsDirent || strlen( rodsDirent->d_name ) == 0 ) {
                free( rodsDirent );
                break;
            }

            if ( strcmp( rodsDirent->d_name, "." ) == 0 ||
                    strcmp( rodsDirent->d_name, ".." ) == 0 ) {
                free( rodsDirent ); // JMC - backport 4835
                continue;
            }

            subPhyPathRegInp = *phyPathRegInp;
            snprintf( subPhyPathRegInp.objPath, MAX_NAME_LEN, "%s/%s",
                      phyPathRegInp->objPath, rodsDirent->d_name );

            if ( matchPathname( ExcludePatterns, rodsDirent->d_name, filePath ) ) {
                free( rodsDirent );
                continue;
            }

            fileStatInp_t fileStatInp;
            memset( &fileStatInp, 0, sizeof( fileStatInp ) );

            // Issue #3658 - This section removes trailing slashes from the
            // directory path in the server when the path is already in the catalog.
            //
            // TODO:  This is a localized fix that addresses any trailing slashes
            // in inPath. Any future refactoring of the code that deals with paths
            // would most probably take care of this issue, and the code segment
            // below would/should then be removed.

            char tmpStr[MAX_NAME_LEN];
            rstrcpy( tmpStr, filePath, MAX_NAME_LEN );
            size_t len = strlen(tmpStr);

            for (size_t i = len-1; i > 0 && tmpStr[i] == '/'; i--) {
                tmpStr[i] = '\0';
            }

            snprintf( fileStatInp.fileName, MAX_NAME_LEN, "%s/%s", tmpStr, rodsDirent->d_name );

            rstrcpy( fileStatInp.objPath, subPhyPathRegInp.objPath, MAX_NAME_LEN );
            fileStatInp.addr = fileOpendirInp.addr;
            rstrcpy( fileStatInp.rescHier, resc_hier, MAX_NAME_LEN );

            rodsStat_t *myStat = NULL;
            status = rsFileStat( rsComm, &fileStatInp, &myStat );

            if ( status != 0 ) {
                rodsLog( LOG_ERROR,
                         "dirPathReg: rsFileStat failed for %s, status = %d",
                         fileStatInp.fileName, status );
                free( rodsDirent ); // JMC - backport 4835
                return status;
            }

            if ( ( myStat->st_mode & S_IFREG ) != 0 ) { /* a file */
                if ( forceFlag > 0 ) {
                    /* check if it already exists */
                    if ( isData( rsComm, subPhyPathRegInp.objPath, NULL ) >= 0 ) {
                        free( myStat );
                        free( rodsDirent ); // JMC - backport 4835
                        continue;
                    }
                }

                subPhyPathRegInp.dataSize = myStat->st_size;
                std::string reg_func_name;

                if ( getValByKey( &phyPathRegInp->condInput, REG_REPL_KW ) != NULL ) {
                    reg_func_name = "filePathRegRepl";
                    status = filePathRegRepl( rsComm, &subPhyPathRegInp, fileStatInp.fileName, _resc_name );
                }
                else {
                    reg_func_name = "filePathReg";
                    addKeyVal( &subPhyPathRegInp.condInput, FILE_PATH_KW, fileStatInp.fileName );
                    status = filePathReg( rsComm, &subPhyPathRegInp, _resc_name );
                }

                if ( status != 0 ) {
                    if ( rsComm->rError.len < MAX_ERROR_MESSAGES ) {
                        rodsLog(LOG_ERROR, "[%s:%d] - adding error to stack...", __FUNCTION__, __LINE__);
                        char error_msg[ERR_MSG_LEN];
                        snprintf( error_msg, ERR_MSG_LEN, "dirPathReg: %s failed for %s, status = %d",
                                 reg_func_name.c_str(), subPhyPathRegInp.objPath, status );
                        addRErrorMsg( &rsComm->rError, status, error_msg );
                    }
                }
            }
            else if ( ( myStat->st_mode & S_IFDIR ) != 0 ) {    /* a directory */
                dirPathReg( rsComm, &subPhyPathRegInp, fileStatInp.fileName, _resc_name );
            }

            free( myStat );
            free( rodsDirent ); // JMC - backport 4835
        }

        if ( status == -1 ) {       /* just EOF */
            status = 0;
        }

        fileClosedirInp.fileInx = dirFd;
        rsFileClosedir( rsComm, &fileClosedirInp );

        return status;
    } // dirPathReg

    int mountFileDir(
        rsComm_t*     rsComm,
        dataObjInp_t* phyPathRegInp,
        char*         filePath,
        const char *rescVaultPath)
    {
        collInp_t collCreateInp;
        int status;
        fileStatInp_t fileStatInp;
        rodsStat_t *myStat = NULL;
        rodsObjStat_t *rodsObjStatOut = NULL;

        char* resc_hier = getValByKey( &phyPathRegInp->condInput, RESC_HIER_STR_KW );
        if ( !resc_hier ) {
            rodsLog( LOG_NOTICE, "mountFileDir - RESC_HIER_STR_KW is NULL" );
            return SYS_INVALID_INPUT_PARAM;
        }

        // =-=-=-=-=-=-=-
        // extract the host location from the resource hierarchy
        std::string location;
        irods::error ret = irods::get_loc_for_hier_string( resc_hier, location );
        if ( !ret.ok() ) {
            irods::log( PASSMSG( "mountFileDir - failed in get_loc_for_hier_string", ret ) );
            return ret.code();
        }

        if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) { // JMC - backport 4832
            rodsLog( LOG_NOTICE, "mountFileDir - insufficient privilege" );
            return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
        }

        status = collStat( rsComm, phyPathRegInp, &rodsObjStatOut );
        if ( status < 0 || NULL == rodsObjStatOut ) {
            freeRodsObjStat( rodsObjStatOut );
            rodsLog( LOG_NOTICE, "mountFileDir collstat failed." );
            return status; // JMC cppcheck - nullptr
        }

        if ( rodsObjStatOut->specColl != NULL ) {
            freeRodsObjStat( rodsObjStatOut );
            rodsLog( LOG_ERROR,
                     "mountFileDir: %s already mounted", phyPathRegInp->objPath );
            return SYS_MOUNT_MOUNTED_COLL_ERR;
        }
        freeRodsObjStat( rodsObjStatOut );
        rodsObjStatOut = NULL;

        if ( isCollEmpty( rsComm, phyPathRegInp->objPath ) == False ) {
            rodsLog( LOG_ERROR,
                     "mountFileDir: collection %s not empty", phyPathRegInp->objPath );
            return SYS_COLLECTION_NOT_EMPTY;
        }

        memset( &fileStatInp, 0, sizeof( fileStatInp ) );

        rstrcpy( fileStatInp.fileName, filePath, MAX_NAME_LEN );
        rstrcpy( fileStatInp.objPath, phyPathRegInp->objPath, MAX_NAME_LEN );
        rstrcpy( fileStatInp.addr.hostAddr,  location.c_str(), NAME_LEN );
        rstrcpy( fileStatInp.rescHier, resc_hier, MAX_NAME_LEN );


        status = rsFileStat( rsComm, &fileStatInp, &myStat );

        if ( status < 0 ) {
            fileMkdirInp_t fileMkdirInp;

            rodsLog( LOG_NOTICE,
                     "mountFileDir: rsFileStat failed for %s, status = %d, create it",
                     fileStatInp.fileName, status );
            memset( &fileMkdirInp, 0, sizeof( fileMkdirInp ) );
            rstrcpy( fileMkdirInp.dirName, filePath, MAX_NAME_LEN );
            rstrcpy( fileMkdirInp.rescHier, resc_hier, MAX_NAME_LEN );
            fileMkdirInp.mode = getDefDirMode();
            rstrcpy( fileMkdirInp.addr.hostAddr,  location.c_str(), NAME_LEN );
            status = rsFileMkdir( rsComm, &fileMkdirInp );
            if ( status < 0 ) {
                return status;
            }
        }
        else if ( ( myStat->st_mode & S_IFDIR ) == 0 ) {
            rodsLog( LOG_ERROR,
                     "mountFileDir: phyPath %s is not a directory",
                     fileStatInp.fileName );
            free( myStat );
            return USER_FILE_DOES_NOT_EXIST;
        }

        free( myStat );
        /* mk the collection */

        memset( &collCreateInp, 0, sizeof( collCreateInp ) );
        rstrcpy( collCreateInp.collName, phyPathRegInp->objPath, MAX_NAME_LEN );
        addKeyVal( &collCreateInp.condInput, COLLECTION_TYPE_KW, MOUNT_POINT_STR );

        addKeyVal( &collCreateInp.condInput, COLLECTION_INFO1_KW, filePath );
        addKeyVal( &collCreateInp.condInput, COLLECTION_INFO2_KW, resc_hier );

        /* try to mod the coll first */
        status = rsModColl( rsComm, &collCreateInp );

        if ( status < 0 ) {    /* try to create it */
            rodsLog( LOG_NOTICE, "mountFileDir rsModColl < 0." );
            status = rsRegColl( rsComm, &collCreateInp );
        }

        if ( status >= 0 ) {
            rodsLog( LOG_NOTICE, "mountFileDir rsModColl > 0." );

            char outLogPath[MAX_NAME_LEN];
            int status1;
            /* see if the phyPath is mapped into a real collection */
            if ( getLogPathFromPhyPath( filePath, rescVaultPath, outLogPath ) >= 0 &&
                    strcmp( outLogPath, phyPathRegInp->objPath ) != 0 ) {
                /* log path not the same as input objPath */
                if ( isColl( rsComm, outLogPath, NULL ) >= 0 ) {
                    modAccessControlInp_t modAccessControl;
                    /* it is a real collection. better set the collection
                     * to read-only mode because any modification to files
                     * through this mounted collection can be trouble */
                    bzero( &modAccessControl, sizeof( modAccessControl ) );
                    modAccessControl.accessLevel = "read";
                    modAccessControl.userName = rsComm->clientUser.userName;
                    modAccessControl.zone = rsComm->clientUser.rodsZone;
                    modAccessControl.path = phyPathRegInp->objPath;
                    status1 = rsModAccessControl( rsComm, &modAccessControl );
                    if ( status1 < 0 ) {
                        rodsLog( LOG_NOTICE,
                                 "mountFileDir: rsModAccessControl err for %s, stat = %d",
                                 phyPathRegInp->objPath, status1 );
                    }
                }
            }
        }

        rodsLog( LOG_NOTICE, "mountFileDir return status." );
        return status;
    } // mountFileDir

    int unmountFileDir(
        rsComm_t *rsComm,
        dataObjInp_t *phyPathRegInp)
    {
        int status;
        collInp_t modCollInp;
        rodsObjStat_t *rodsObjStatOut = NULL;

        status = collStat( rsComm, phyPathRegInp, &rodsObjStatOut );
        if ( status < 0 || NULL == rodsObjStatOut ) { // JMC cppcheck - nullptr
            free( rodsObjStatOut );
            return status;
        }
        else if ( rodsObjStatOut->specColl == NULL ) {
            freeRodsObjStat( rodsObjStatOut );
            rodsLog( LOG_ERROR,
                     "unmountFileDir: %s not mounted", phyPathRegInp->objPath );
            return SYS_COLL_NOT_MOUNTED_ERR;
        }

        if ( getStructFileType( rodsObjStatOut->specColl ) >= 0 ) {
            status = _rsSyncMountedColl( rsComm, rodsObjStatOut->specColl,
                                         PURGE_STRUCT_FILE_CACHE );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR, "unmountFileDir -  failed in _rsSyncMountedColl with status %d", status );
            }
        }

        freeRodsObjStat( rodsObjStatOut );
        rodsObjStatOut = NULL;

        memset( &modCollInp, 0, sizeof( modCollInp ) );
        rstrcpy( modCollInp.collName, phyPathRegInp->objPath, MAX_NAME_LEN );
        addKeyVal( &modCollInp.condInput, COLLECTION_TYPE_KW,
                   "NULL_SPECIAL_VALUE" );
        addKeyVal( &modCollInp.condInput, COLLECTION_INFO1_KW, "NULL_SPECIAL_VALUE" );
        addKeyVal( &modCollInp.condInput, COLLECTION_INFO2_KW, "NULL_SPECIAL_VALUE" );

        status = rsModColl( rsComm, &modCollInp );

        return status;
    } // unmountFileDir

    int structFileReg(
        rsComm_t*     rsComm,
        dataObjInp_t* phyPathRegInp)
    {
        // =-=-=-=-=-=-=-
        //
        dataObjInp_t     dataObjInp;
        collInp_t        collCreateInp;
        int              status         = 0;
        int              len            = 0;
        char*            collType       = NULL;
        char*            structFilePath = NULL;
        dataObjInfo_t*   dataObjInfo    = NULL;
        rodsObjStat_t*   rodsObjStatOut = NULL;
        specCollCache_t* specCollCache  = NULL;

        if ( ( structFilePath = getValByKey( &phyPathRegInp->condInput, FILE_PATH_KW ) )
                == NULL ) {
            rodsLog( LOG_ERROR,
                     "structFileReg: No structFilePath input for %s",
                     phyPathRegInp->objPath );
            return SYS_INVALID_FILE_PATH;
        }

        collType = getValByKey( &phyPathRegInp->condInput, COLLECTION_TYPE_KW );
        if ( collType == NULL ) {
            rodsLog( LOG_ERROR,
                     "structFileReg: Bad COLLECTION_TYPE_KW for structFilePath %s",
                     dataObjInp.objPath );
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }

        len = strlen( phyPathRegInp->objPath );
        if ( strncmp( structFilePath, phyPathRegInp->objPath, len ) == 0 &&
                ( structFilePath[len] == '\0' || structFilePath[len] == '/' ) ) {
            rodsLog( LOG_ERROR,
                     "structFileReg: structFilePath %s inside collection %s",
                     structFilePath, phyPathRegInp->objPath );
            return SYS_STRUCT_FILE_INMOUNTED_COLL;
        }

        /* see if the struct file is in spec coll */

        if ( getSpecCollCache( rsComm, structFilePath, 0,  &specCollCache ) >= 0 ) {
            rodsLog( LOG_ERROR,
                     "structFileReg: structFilePath %s is in a mounted path",
                     structFilePath );
            return SYS_STRUCT_FILE_INMOUNTED_COLL;
        }

        status = collStat( rsComm, phyPathRegInp, &rodsObjStatOut );
        if ( status < 0 || NULL == rodsObjStatOut ) {
            free( rodsObjStatOut );
            return status;    // JMC cppcheck - nullptr
        }

        if ( rodsObjStatOut->specColl != NULL ) {
            freeRodsObjStat( rodsObjStatOut );
            rodsLog( LOG_ERROR,
                     "structFileReg: %s already mounted", phyPathRegInp->objPath );
            return SYS_MOUNT_MOUNTED_COLL_ERR;
        }

        freeRodsObjStat( rodsObjStatOut );
        rodsObjStatOut = NULL;

        if ( isCollEmpty( rsComm, phyPathRegInp->objPath ) == False ) {
            rodsLog( LOG_ERROR,
                     "structFileReg: collection %s not empty", phyPathRegInp->objPath );
            return SYS_COLLECTION_NOT_EMPTY;
        }

        memset( &dataObjInp, 0, sizeof( dataObjInp ) );
        rstrcpy( dataObjInp.objPath, structFilePath, sizeof( dataObjInp.objPath ) );
        /* user need to have write permission */
        dataObjInp.openFlags = O_WRONLY;
        status = getDataObjInfoIncSpecColl( rsComm, &dataObjInp, &dataObjInfo );
        if ( status < 0 || NULL == dataObjInfo ) { // JMC cppcheck - nullptr
            int myStatus;
            /* try to make one */
            dataObjInp.condInput = phyPathRegInp->condInput;
            /* have to remove FILE_PATH_KW because getFullPathName will use it */
            rmKeyVal( &dataObjInp.condInput, FILE_PATH_KW );
            myStatus = rsDataObjCreate( rsComm, &dataObjInp );
            if ( myStatus < 0 ) {
                rodsLog( LOG_ERROR,
                         "structFileReg: Problem with open/create structFilePath %s, status = %d",
                         dataObjInp.objPath, status );
                return status;
            }
            else {
                openedDataObjInp_t dataObjCloseInp;
                bzero( &dataObjCloseInp, sizeof( dataObjCloseInp ) );
                dataObjCloseInp.l1descInx = myStatus;
                rsDataObjClose( rsComm, &dataObjCloseInp );
            }
        }

        char* tmp_hier = getValByKey( &phyPathRegInp->condInput, RESC_HIER_STR_KW );
        if ( !tmp_hier ) {
            rodsLog( LOG_ERROR, "structFileReg - RESC_HIER_STR_KW is NULL" );
            return SYS_INVALID_INPUT_PARAM;
        }
        irods::hierarchy_parser parser;
        parser.set_string( std::string( tmp_hier ) );
        std::string resc_name;
        parser.last_resc( resc_name );

        /* mk the collection */

        memset( &collCreateInp, 0, sizeof( collCreateInp ) );
        rstrcpy( collCreateInp.collName, phyPathRegInp->objPath, MAX_NAME_LEN );
        addKeyVal( &collCreateInp.condInput, COLLECTION_TYPE_KW, collType );
        /* have to use dataObjInp.objPath because structFile path was removed */
        addKeyVal( &collCreateInp.condInput, COLLECTION_INFO1_KW, dataObjInp.objPath );

        /* try to mod the coll first */
        status = rsModColl( rsComm, &collCreateInp );

        if ( status < 0 ) { /* try to create it */
            status = rsRegColl( rsComm, &collCreateInp );
        }

        return status;
    } // structFileReg

    int structFileSupport(
        rsComm_t *rsComm,
        char *collection,
        char *collType,
        char* resc_hier)
    {
        int status;
        subFile_t subFile;
        specColl_t specColl;

        if ( rsComm == NULL || collection == NULL || collType == NULL ||
                resc_hier == NULL ) {
            return 0;
        }

        memset( &subFile, 0, sizeof( subFile ) );
        memset( &specColl, 0, sizeof( specColl ) );
        /* put in some fake path */
        subFile.specColl = &specColl;
        rstrcpy( specColl.collection, collection, MAX_NAME_LEN );
        specColl.collClass = STRUCT_FILE_COLL;
        if ( strcmp( collType, HAAW_STRUCT_FILE_STR ) == 0 ) {
            specColl.type = HAAW_STRUCT_FILE_T;
        }
        else if ( strcmp( collType, TAR_STRUCT_FILE_STR ) == 0 ) {
            specColl.type = TAR_STRUCT_FILE_T;
        }
        else if ( strcmp( collType, MSSO_STRUCT_FILE_STR ) == 0 ) {
            specColl.type = MSSO_STRUCT_FILE_T;
        }
        else {
            return 0;
        }

        // =-=-=-=-=-=-=-
        // extract the host location from the resource hierarchy
        std::string location;
        irods::error ret = irods::get_loc_for_hier_string( resc_hier, location );
        if ( !ret.ok() ) {
            irods::log( PASSMSG( "structFileSupport - failed in get_loc_for_hier_string", ret ) );
            return ret.code();
        }

        irods::hierarchy_parser parser;
        parser.set_string( resc_hier );
        std::string first_resc;
        parser.first_resc( first_resc );

        snprintf( specColl.objPath, MAX_NAME_LEN, "%s/myFakeFile", collection );
        rstrcpy( specColl.resource, first_resc.c_str(), NAME_LEN );
        rstrcpy( specColl.rescHier, resc_hier, MAX_NAME_LEN );
        rstrcpy( specColl.phyPath, "/fakeDir1/fakeDir2/myFakeStructFile", MAX_NAME_LEN );
        rstrcpy( subFile.subFilePath, "/fakeDir1/fakeDir2/myFakeFile", MAX_NAME_LEN );
        rstrcpy( subFile.addr.hostAddr, location.c_str(), NAME_LEN );

        rodsStat_t *myStat = NULL;
        status = rsSubStructFileStat( rsComm, &subFile, &myStat );
        free( myStat );
        return status != SYS_NOT_SUPPORTED;
    } // structFileSupport

    int linkCollReg(
        rsComm_t *rsComm,
        dataObjInp_t *phyPathRegInp)
    {
        collInp_t collCreateInp;
        int status;
        char *linkPath = NULL;
        char *collType;
        int len;
        rodsObjStat_t *rodsObjStatOut = NULL;
        specCollCache_t *specCollCache = NULL;

        if ( ( linkPath = getValByKey( &phyPathRegInp->condInput, FILE_PATH_KW ) )
                == NULL ) {
            rodsLog( LOG_ERROR,
                     "linkCollReg: No linkPath input for %s",
                     phyPathRegInp->objPath );
            return SYS_INVALID_FILE_PATH;
        }

        collType = getValByKey( &phyPathRegInp->condInput, COLLECTION_TYPE_KW );
        if ( collType == NULL || strcmp( collType, LINK_POINT_STR ) != 0 ) {
            rodsLog( LOG_ERROR,
                     "linkCollReg: Bad COLLECTION_TYPE_KW for linkPath %s",
                     phyPathRegInp->objPath );
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }

        if ( phyPathRegInp->objPath[0] != '/' || linkPath[0] != '/' ) {
            rodsLog( LOG_ERROR,
                     "linkCollReg: linkPath %s or collection %s not absolute path",
                     linkPath, phyPathRegInp->objPath );
            return SYS_COLL_LINK_PATH_ERR;
        }

        len = strlen( phyPathRegInp->objPath );
        if ( strncmp( linkPath, phyPathRegInp->objPath, len ) == 0 &&
                linkPath[len] == '/' ) {
            rodsLog( LOG_ERROR,
                     "linkCollReg: linkPath %s inside collection %s",
                     linkPath, phyPathRegInp->objPath );
            return SYS_COLL_LINK_PATH_ERR;
        }

        len = strlen( linkPath );
        if ( strncmp( phyPathRegInp->objPath, linkPath, len ) == 0 &&
                phyPathRegInp->objPath[len] == '/' ) {
            rodsLog( LOG_ERROR,
                     "linkCollReg: collection %s inside linkPath %s",
                     linkPath, phyPathRegInp->objPath );
            return SYS_COLL_LINK_PATH_ERR;
        }

        if ( getSpecCollCache( rsComm, linkPath, 0,  &specCollCache ) >= 0 &&
                specCollCache->specColl.collClass != LINKED_COLL ) {
            rodsLog( LOG_ERROR,
                     "linkCollReg: linkPath %s is in a spec coll path",
                     linkPath );
            return SYS_COLL_LINK_PATH_ERR;
        }

        status = collStat( rsComm, phyPathRegInp, &rodsObjStatOut );
        if ( status < 0 ) {
            freeRodsObjStat( rodsObjStatOut );
            rodsObjStatOut = NULL;
            /* does not exist. make one */
            collInp_t collCreateInp;
            memset( &collCreateInp, 0, sizeof( collCreateInp ) );
            rstrcpy( collCreateInp.collName, phyPathRegInp->objPath, MAX_NAME_LEN );
            status = rsRegColl( rsComm, &collCreateInp );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "linkCollReg: rsRegColl error for  %s, status = %d",
                         collCreateInp.collName, status );
                return status;
            }
            status = collStat( rsComm, phyPathRegInp, &rodsObjStatOut );
            if ( status < 0 ) {
                freeRodsObjStat( rodsObjStatOut );
                return status;
            }

        }

        if ( rodsObjStatOut && // JMC cppcheck - nullptr
                rodsObjStatOut->specColl != NULL &&
                rodsObjStatOut->specColl->collClass != LINKED_COLL ) {
            freeRodsObjStat( rodsObjStatOut );
            rodsLog( LOG_ERROR,
                     "linkCollReg: link collection %s in a spec coll path",
                     phyPathRegInp->objPath );
            return SYS_COLL_LINK_PATH_ERR;
        }

        freeRodsObjStat( rodsObjStatOut );
        rodsObjStatOut = NULL;

        if ( isCollEmpty( rsComm, phyPathRegInp->objPath ) == False ) {
            rodsLog( LOG_ERROR,
                     "linkCollReg: collection %s not empty", phyPathRegInp->objPath );
            return SYS_COLLECTION_NOT_EMPTY;
        }

        /* mk the collection */

        memset( &collCreateInp, 0, sizeof( collCreateInp ) );
        rstrcpy( collCreateInp.collName, phyPathRegInp->objPath, MAX_NAME_LEN );
        addKeyVal( &collCreateInp.condInput, COLLECTION_TYPE_KW, collType );

        /* have to use dataObjInp.objPath because structFile path was removed */
        addKeyVal( &collCreateInp.condInput, COLLECTION_INFO1_KW,
                   linkPath );

        /* try to mod the coll first */
        status = rsModColl( rsComm, &collCreateInp );

        if ( status < 0 ) { /* try to create it */
            status = rsRegColl( rsComm, &collCreateInp );
        }

        return status;
    } // linkCollReg

    int _rsPhyPathReg(
        rsComm_t *rsComm,
        dataObjInp_t *phyPathRegInp,
        const char *_resc_name,
        rodsServerHost_t *rodsServerHost)
    {
        int status = 0;
        fileOpenInp_t chkNVPathPermInp;
        char *tmpFilePath = 0;
        char filePath[MAX_NAME_LEN];
        dataObjInfo_t dataObjInfo;
        char *tmpStr = NULL;
        int chkType = 0; // JMC - backport 4774
        char *excludePatternFile = 0;

        if ( ( tmpFilePath = getValByKey( &phyPathRegInp->condInput, FILE_PATH_KW ) ) == NULL ) {
            rodsLog( LOG_ERROR, "_rsPhyPathReg: No filePath input for %s",
                     phyPathRegInp->objPath );
            return SYS_INVALID_FILE_PATH;
        }
        else {
            /* have to do this since it will be over written later */
            rstrcpy( filePath, tmpFilePath, MAX_NAME_LEN );
        }

        /* check if we need to chk permission */
        memset( &dataObjInfo, 0, sizeof( dataObjInfo ) );
        rstrcpy( dataObjInfo.objPath, phyPathRegInp->objPath, MAX_NAME_LEN );
        rstrcpy( dataObjInfo.filePath, filePath, MAX_NAME_LEN );
        rstrcpy( dataObjInfo.rescName, _resc_name, NAME_LEN );

        char* resc_hier = getValByKey( &phyPathRegInp->condInput, RESC_HIER_STR_KW );
        if ( resc_hier ) {
            rstrcpy( dataObjInfo.rescHier, resc_hier, MAX_NAME_LEN );
        }
        else {
            rodsLog( LOG_NOTICE, "_rsPhyPathReg :: RESC_HIER_STR_KW is NULL" );
            return SYS_INVALID_INPUT_PARAM;
        }

        irods::error ret = resc_mgr.hier_to_leaf_id(resc_hier,dataObjInfo.rescId);
        if( !ret.ok() ) {
            irods::log(PASS(ret));
        }

        if ( getValByKey( &phyPathRegInp->condInput, NO_CHK_FILE_PERM_KW ) == NULL &&
                ( chkType = getchkPathPerm( rsComm, phyPathRegInp, &dataObjInfo ) ) != NO_CHK_PATH_PERM ) { // JMC - backport 4774

            memset( &chkNVPathPermInp, 0, sizeof( chkNVPathPermInp ) );

            rstrcpy( chkNVPathPermInp.fileName, filePath, MAX_NAME_LEN );

            // =-=-=-=-=-=-=-
            // extract the host location from the resource hierarchy
            std::string location;
            irods::error ret = irods::get_loc_for_hier_string( resc_hier, location );
            if ( !ret.ok() ) {
                irods::log( PASSMSG( "_rsPhyPathReg - failed in get_loc_for_hier_string", ret ) );
                return ret.code();
            }

            rstrcpy( chkNVPathPermInp.addr.hostAddr, location.c_str(), NAME_LEN );
            status = chkFilePathPerm( rsComm, &chkNVPathPermInp, rodsServerHost, chkType ); // JMC - backport 4774

            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "_rsPhyPathReg: chkFilePathPerm error for %s",
                         phyPathRegInp->objPath );
                return status;
            }
        }

        if ( getValByKey( &phyPathRegInp->condInput, COLLECTION_KW ) != NULL ) {
            excludePatternFile = getValByKey( &phyPathRegInp->condInput, EXCLUDE_FILE_KW );
            if ( excludePatternFile != NULL ) {
                ExcludePatterns = readPathnamePatternsFromFile( rsComm,
                                  excludePatternFile,
                                  resc_hier );
            }

            status = dirPathReg( rsComm, phyPathRegInp, filePath, _resc_name );
            if ( excludePatternFile != NULL ) {
                freePathnamePatterns( ExcludePatterns );
                ExcludePatterns = NULL;
            }

        }
        else if ( ( tmpStr = getValByKey( &phyPathRegInp->condInput, COLLECTION_TYPE_KW ) ) != NULL && strcmp( tmpStr, MOUNT_POINT_STR ) == 0 ) {
            rodsLong_t resc_id = 0;
            ret = resc_mgr.hier_to_leaf_id(_resc_name,resc_id);
            if(!ret.ok()) {
                return ret.code();
            }

            // Get resource path
            std::string resc_vault_path;
            irods::error ret = irods::get_resource_property< std::string >( resc_id, irods::RESOURCE_PATH, resc_vault_path );
            if ( !ret.ok() ) {
                irods::log PASSMSG( "_rsPhyPathReg - failed in get_resource_property", ret );
                return ret.code();
            }

            status = mountFileDir( rsComm, phyPathRegInp, filePath, resc_vault_path.c_str() );

        }
        else {
            bool register_replica = false;

            // This flag is set by rsDataObjOpen to allow creation of replicas via the streaming
            // interface (e.g. dstream).
            if (irods::experimental::key_value_proxy{rsComm->session_props}.contains(REG_REPL_KW)) {
                // At this point, we know that the target resource does not contain a replica.
                // If at least one replica exists, then the file needs to be registered instead of
                // creating a new data object.
                const irods::experimental::filesystem::path p = phyPathRegInp->objPath;

                const auto gql = fmt::format("select DATA_ID where COLL_NAME = '{}' and DATA_NAME = '{}'",
                    p.parent_path().c_str(),
                    p.object_name().c_str());

                register_replica = irods::query{rsComm, gql}.size() > 0;
            }

            if (register_replica || getValByKey(&phyPathRegInp->condInput, REG_REPL_KW)) {
                status = filePathRegRepl( rsComm, phyPathRegInp, filePath, _resc_name );
            }
            else {
                status = filePathReg( rsComm, phyPathRegInp, _resc_name );
            }
        }

        return status;
    } // _rsPhyPathReg

} // anonymous namespace

int rsPhyPathReg(
    rsComm_t *rsComm,
    dataObjInp_t *phyPathRegInp)
{
    if ( getValByKey( &phyPathRegInp->condInput, NO_CHK_FILE_PERM_KW ) != NULL &&
            rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return SYS_NO_API_PRIV;
    }

    namespace fs = irods::experimental::filesystem;

    // Also covers checking of empty paths.
    if (fs::path{phyPathRegInp->objPath}.object_name().empty()) {
        return USER_INPUT_PATH_ERR;
    }

    rodsServerHost_t *rodsServerHost = NULL;
    int remoteFlag;
    rodsHostAddr_t addr;

    // =-=-=-=-=-=-=-
    // NOTE:: resource_redirect can wipe out the specColl due to a call to getDataObjIncSpecColl
    //        which nullifies the specColl in the case of certain special collections ( LINKED ).
    //        this block of code needs to be called before redirect to handle that case as it doesnt
    //        need the resource hierarchy anyway.  this is the sort of thing i'd like to avoid in
    //        the future

    char* coll_type = getValByKey( &phyPathRegInp->condInput, COLLECTION_TYPE_KW );

    if ( coll_type && strcmp( coll_type, UNMOUNT_STR ) == 0 ) {
        return unmountFileDir( rsComm, phyPathRegInp );
    }
    else  if ( coll_type != NULL && strcmp( coll_type, LINK_POINT_STR ) == 0 ) {
        return linkCollReg( rsComm, phyPathRegInp );
    }

    // =-=-=-=-=-=-=-
    // determine if a hierarchy has been passed by kvp, if so use it.
    // otherwise determine a resource hierarchy given dst hier or default resource
    std::string hier;
    char*       tmp_hier = getValByKey( &phyPathRegInp->condInput, RESC_HIER_STR_KW );
    if ( NULL == tmp_hier ) {
        // =-=-=-=-=-=-=-
        // if no hier is provided, determine if a resource was specified
        char* dst_resc = getValByKey( &phyPathRegInp->condInput, DEST_RESC_NAME_KW );
        if ( dst_resc ) {
            // =-=-=-=-=-=-=-
            // if we do have a dest resc, see if it has a parent or a child
            irods::resource_ptr resc;
            irods::error ret = resc_mgr.resolve( dst_resc, resc );
            if ( !ret.ok() ) {
                irods::log( PASS( ret ) );
                return ret.code();
            }

            // =-=-=-=-=-=-=-
            // get parent
            irods::resource_ptr parent_resc;
            ret = resc->get_parent( parent_resc );
            bool has_parent = ret.ok();

            // =-=-=-=-=-=-=-
            // get child
            bool has_child = ( resc->num_children() > 0 );

            // =-=-=-=-=-=-=-
            // if the resc is mid-tier this is a Bad Thing
            if ( has_parent && has_child ) {
                return HIERARCHY_ERROR;
            }
            // =-=-=-=-=-=-=-
            // this is a leaf node situation
            else if ( has_parent && !has_child ) {
                // =-=-=-=-=-=-=-
                // get the path from our parent resource
                // to this given leaf resource - this our hier
                ret = resc_mgr.get_hier_to_root_for_resc(
                        dst_resc,
                        hier );
                if(!ret.ok()) {
                    irods::log(PASS(ret));
                    return ret.code();
                }

                addKeyVal(
                    &phyPathRegInp->condInput,
                    RESC_HIER_STR_KW,
                    hier.c_str() );

                // =-=-=-=-=-=-=-
                // get the root resc of the hier, this is the
                // new resource name
                std::string root_resc;
                irods::hierarchy_parser parser;
                parser.set_string( hier );
                parser.first_resc( root_resc );
                addKeyVal(
                    &phyPathRegInp->condInput,
                    DEST_RESC_NAME_KW,
                    root_resc.c_str() );

            }
            // =-=-=-=-=-=-=-
            // this is a solo node situation
            else if ( !has_parent && !has_child ) {
                hier = dst_resc;
                addKeyVal(
                    &phyPathRegInp->condInput,
                    RESC_HIER_STR_KW,
                    hier.c_str() );
            }
            // =-=-=-=-=-=-=-
            // root node and pathological situation
            else {
                try {
                    auto result = irods::resolve_resource_hierarchy(irods::CREATE_OPERATION, rsComm, *phyPathRegInp);
                    hier = std::get<std::string>(result);
                    addKeyVal( &phyPathRegInp->condInput, RESC_HIER_STR_KW, hier.c_str() );
                }
                catch (const irods::exception& e) {
                    irods::log(e);
                    return e.code();
                }
            }
        } // if dst_resc
        else {
            // =-=-=-=-=-=-=-
            // no resc is specified, request a hierarchy given the default resource
            try {
                auto result = irods::resolve_resource_hierarchy(irods::CREATE_OPERATION, rsComm, *phyPathRegInp);
                hier = std::get<std::string>(result);
            }
            catch (const irods::exception& e) {
                irods::log(e);
                return e.code();
            }
            addKeyVal( &phyPathRegInp->condInput, RESC_HIER_STR_KW, hier.c_str() );
        } // else
    } // if tmp_hier
    else {
        hier = tmp_hier;
    }

    // =-=-=-=-=-=-=-
    // coll registration requires the resource hierarchy
    if ( coll_type && ( strcmp( coll_type, HAAW_STRUCT_FILE_STR ) == 0 ||
                        strcmp( coll_type, TAR_STRUCT_FILE_STR ) == 0 ||
                        strcmp( coll_type, MSSO_STRUCT_FILE_STR ) == 0 ) ) {
        return structFileReg( rsComm, phyPathRegInp );
    }

    // =-=-=-=-=-=-=-
    rodsLong_t resc_id = 0;
    irods::error ret = resc_mgr.hier_to_leaf_id(hier,resc_id);
    if(!ret.ok()) {
        return ret.code();
    }

    std::string location;
    ret = irods::get_resource_property< std::string >(
              resc_id,
              irods::RESOURCE_LOCATION,
              location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "failed in get_resource_property", ret ) );
        return ret.code();
    }

    memset( &addr, 0, sizeof( addr ) );
    rstrcpy( addr.hostAddr, location.c_str(), LONG_NAME_LEN );
    remoteFlag = resolveHost( &addr, &rodsServerHost );

    // We do not need to redirect if we do not need to stat the file which is to be registered
    const auto size_kw{getValByKey(&phyPathRegInp->condInput, DATA_SIZE_KW)};
    if ( size_kw || remoteFlag == LOCAL_HOST ) {
        irods::hierarchy_parser p;
        p.set_string(hier);
        std::string leaf_resc;
        p.last_resc(leaf_resc);
        return _rsPhyPathReg( rsComm, phyPathRegInp, leaf_resc.c_str(), rodsServerHost );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        return remotePhyPathReg( rsComm, phyPathRegInp, rodsServerHost );
    }

    if ( remoteFlag < 0 ) {
        return remoteFlag;
    }

    rodsLog( LOG_ERROR,
             "rsPhyPathReg: resolveHost returned unrecognized value %d",
             remoteFlag );
    return SYS_UNRECOGNIZED_REMOTE_FLAG;
} // rsPhyPathReg

