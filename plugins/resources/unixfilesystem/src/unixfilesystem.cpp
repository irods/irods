// =-=-=-=-=-=-=-
// irods includes
#include "irods/msParam.h"
#include "irods/rcConnect.h"
#include "irods/miscServerFunct.hpp"
#include "irods/generalAdmin.h"

// =-=-=-=-=-=-=-
#include "irods/irods_resource_plugin.hpp"
#include "irods/irods_resource_backport.hpp"
#include "irods/irods_file_object.hpp"
#include "irods/irods_physical_object.hpp"
#include "irods/irods_collection_object.hpp"
#include "irods/irods_string_tokenize.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/irods_resource_redirect.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/irods_kvp_string_parser.hpp"
#include "irods/irods_logger.hpp"
#include "irods/voting.hpp"
#include "irods/private/irods_is_in_host_list.hpp"
#include "irods/filesystem/path.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <optional>

// =-=-=-=-=-=-=-
// boost includes
#include <boost/function.hpp>
#include <boost/any.hpp>
#include <boost/filesystem.hpp>

// =-=-=-=-=-=-=-
// system includes
#ifndef _WIN32
#include <sys/file.h>
#include <sys/param.h>
#endif
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/types.h>
#if defined(osx_platform)
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif
#include <fcntl.h>
#ifndef _WIN32
#include <sys/file.h>
#include <unistd.h>
#endif
#include <dirent.h>

#include <fmt/format.h>

#if defined(solaris_platform)
#include <sys/statvfs.h>
#endif

#if defined(osx_platform)
#include <sys/param.h>
#include <sys/mount.h>
#endif

#if defined(linux_platform)
#include <sys/vfs.h>
#endif
#include <sys/stat.h>

#include <string.h>
#include <sstream>


// =-=-=-=-=-=-=-
// 1. Define utility functions that the operations might need
const std::string DEFAULT_VAULT_DIR_MODE( "default_vault_directory_mode_kw" );
const std::string HIGH_WATER_MARK( "high_water_mark" ); // no longer used
const std::string REQUIRED_FREE_INODES_FOR_CREATE("required_free_inodes_for_create"); // no longer used
const std::string HOST_MODE("host_mode");

namespace
{
    auto is_operating_in_detached_mode(irods::plugin_property_map& prop_map) -> bool
    {
        std::string host_mode_str;

        irods::error ret = prop_map.get<std::string>(HOST_MODE, host_mode_str);
        if (ret.ok()) {
            return boost::iequals(host_mode_str.c_str(), "detached");
        }

        // default is attached mode
        return false;
    }
} // namespace

// The return value is always SUCCESS() because the errors encountered are to
// be treated as warnings and no fatal errors.
auto unix_file_start_operation(irods::plugin_property_map& prop_map) -> irods::error
{
    namespace logger = irods::experimental::log;

    if (!is_operating_in_detached_mode(prop_map)) {
        // Detached mode is disabled.  Return early.
        return SUCCESS();
    }

    // check if we are in the host list
    char local_hostname[MAX_NAME_LEN];
    gethostname(local_hostname, MAX_NAME_LEN);

    auto resource_hostname_optional = resolve_hostname(local_hostname, hostname_resolution_scheme::match_preferred);

    std::string resource_hostname =
        resource_hostname_optional ? std::move(*resource_hostname_optional) : local_hostname;

    if (!is_host_in_host_list(prop_map, resource_hostname)) {
        // Host is not in host list.  Return early as detached mode is disabled on this resource.
        return SUCCESS();
    }

    // Set the RESOURCE_HOST of this resource to the rodsHostAddr_t for this server.
    // This server will now act as the owner of this resource for any processing on
    // this server.

    // Note:  On error conditions below, detached mode will remain disabled and a warning
    // will be logged.

    std::string resource_name;
    irods::error ret = prop_map.get<std::string>(irods::RESOURCE_NAME, resource_name);
    if (!ret.ok()) {
        // Logging a warning as returning an error here simply logs the error
        // and continues.
        logger::resource::warn(
            "Detached mode for unixfilesystem resource failed to set RESOURCE_HOST to {}."
            "  Failed to get irods::RESOURCE_NAME property.",
            local_hostname);
        return SUCCESS();
    }

    // Look up the rodsHostAddr_t for this server and set this as the RESOURCE_HOST of the resource.
    // Note that the rodsHostAddr_t entry is shared throughout the code (not just used for this
    // resource) so you can't adjust the existing entry attached to the resource.
    rodsHostAddr_t addr{};
    std::strncpy(addr.hostAddr, local_hostname, LONG_NAME_LEN);
    const auto& zone_name = irods::get_server_property<const std::string>(irods::KW_CFG_ZONE_NAME);
    std::strncpy(addr.zoneName, zone_name.c_str(), NAME_LEN);

    rodsServerHost_t* local_host = nullptr;
    if (resolveHost(&addr, &local_host) < 0) {
        logger::resource::warn(
            "[resource_name={}] Detached mode failed to set RESOURCE_HOST to {}."
            "  Failed to call resolveHost.",
            resource_name,
            local_hostname);
        return SUCCESS();
    }

    irods::set_resource_property<rodsServerHost_t*>(resource_name, irods::RESOURCE_HOST, local_host);

    return SUCCESS();
} // unix_file_start_operation
// =-=-=-=-=-=-=-
// NOTE: All storage resources must do this on the physical path stored in the file object and then update
//       the file object's physical path with the full path

static irods::error unix_file_copy(
    const int mode,
    const char* srcFileName,
    const char* destFileName)
{
    struct stat statbuf;
    int status = stat( srcFileName, &statbuf );
    int err_status = errno;
    if ( status < 0 ) {
        return ERROR(UNIX_FILE_STAT_ERR, fmt::format(
            "stat failed on \"{}\", status: {}",
            srcFileName, err_status));
    }

    if ( ( statbuf.st_mode & S_IFREG ) == 0 ) {
        return ERROR(UNIX_FILE_STAT_ERR, fmt::format(
            "srcFileName \"{}\" is not a regular file.",
            srcFileName, err_status));
    }

    int inFd{-1};
    int outFd{-1};
    const irods::at_scope_exit close_fds{
        [&]()
        {
            if (inFd > 0) close(inFd);
            if (outFd > 0) close(outFd);
        }
    };

    inFd = open( srcFileName, O_RDONLY, 0 );
    err_status = UNIX_FILE_OPEN_ERR - errno;
    if ( inFd < 0 ) {
        return ERROR(err_status, fmt::format(
            "Open error for srcFileName \"{}\", status = {}",
            srcFileName, err_status));
    }

    outFd = open( destFileName, O_WRONLY | O_CREAT | O_TRUNC, mode );
    err_status = UNIX_FILE_OPEN_ERR - errno;
    if ( outFd < 0 ) {
        return ERROR(err_status, fmt::format(
            "Open error for destFileName \"{}\", status = {}",
            destFileName, err_status));
    }

    size_t trans_buff_size;
    try {
        trans_buff_size = irods::get_advanced_setting<const int>(irods::KW_CFG_TRANS_BUFFER_SIZE_FOR_PARA_TRANS) * 1024 * 1024;
    }
    catch ( const irods::exception& e ) {
        return irods::error(e);
    }

    std::vector<char> myBuf( trans_buff_size );
    int bytesRead{};
    rodsLong_t bytesCopied = 0;
    while ( ( bytesRead = read( inFd, ( void * ) myBuf.data(), trans_buff_size ) ) > 0 ) {
        int bytesWritten = write( outFd, ( void * ) myBuf.data(), bytesRead );
        err_status = UNIX_FILE_WRITE_ERR - errno;
        if (bytesWritten <= 0) {
            irods::log(ERROR(
                err_status, fmt::format(
                    "Write error for destFileName {}, status = {}",
                    destFileName, status)));
            break;
        }
        bytesCopied += bytesWritten;
    }

    if (bytesCopied != statbuf.st_size) {
        return ERROR(SYS_COPY_LEN_ERR, fmt::format(
            "Copied size {} does not match source size {} of {}",
            bytesCopied, statbuf.st_size, srcFileName));
    }

    return SUCCESS();
} // unix_file_copy



/// @brief Generates a full path name from the partial physical path and the specified resource's vault path
irods::error unix_generate_full_path(
    irods::plugin_property_map& _prop_map,
    const std::string&           _phy_path,
    std::string&                 _ret_string)
{
    std::string vault_path;
    if (const auto ret = _prop_map.get<std::string>(irods::RESOURCE_PATH, vault_path); !ret.ok()) {
        return ERROR(SYS_INVALID_INPUT_PARAM, "resource has no vault path.");
    }

    if ( _phy_path.compare( 0, 1, "/" ) != 0 &&
            _phy_path.compare( 0, vault_path.size(), vault_path ) != 0 ) {
        _ret_string  = vault_path;
        _ret_string += "/";
        _ret_string += _phy_path;
    }
    else {
        // The physical path already contains the vault path
        _ret_string = _phy_path;
    }

    return SUCCESS();
} // unix_generate_full_path

/// @brief update the physical path in the file object
irods::error unix_check_path(irods::plugin_context& _ctx)
{
    // try dynamic cast on ptr, throw error otherwise
    irods::data_object_ptr data_obj = boost::dynamic_pointer_cast<irods::data_object>(_ctx.fco());
    if (!data_obj.get()) {
        return ERROR(SYS_INVALID_INPUT_PARAM, "Failed to cast fco to data_object.");
    }

    // NOTE: Must do this for all storage resources
    std::string full_path;
    if (const auto err = unix_generate_full_path(_ctx.prop_map(), data_obj->physical_path(), full_path); !err.ok()) {
        return PASSMSG("Failed generating full path for object.", err);
    }

    data_obj->physical_path( full_path );

    return SUCCESS();
} // unix_check_path

/// @brief Checks the basic operation parameters and updates the physical path in the file object
template<typename DEST_TYPE>
irods::error unix_check_params_and_path(irods::plugin_context& _ctx)
{
    // verify that the resc context is valid
    if (const auto err = _ctx.valid<DEST_TYPE>(); !err.ok()) {
        return PASSMSG("resource context is invalid.", err);
    }

    return unix_check_path(_ctx);
} // unix_check_params_and_path

/// @brief Checks the basic operation parameters and updates the physical path in the file object
irods::error unix_check_params_and_path(irods::plugin_context& _ctx)
{
    // verify that the resc context is valid
    if (const auto err = _ctx.valid(); !err.ok()) {
        return PASSMSG("unix_check_params_and_path - resource context is invalid", err);
    }

    return unix_check_path(_ctx);
} // unix_check_params_and_path

// =-=-=-=-=-=-=-
//@brief Recursively make all of the dirs in the path
irods::error unix_file_mkdir_r(const std::string& path, mode_t mode)
{
    std::string subdir;
    std::size_t pos = 0;
    bool done = false;
    while (!done) {
        pos = path.find_first_of( '/', pos + 1 );
        if ( pos > 0 ) {
            subdir = path.substr( 0, pos );

            // handle error cases
            if (const auto ec = mkdir(subdir.c_str(), mode); ec < 0 && errno != EEXIST) {
                return ERROR(UNIX_FILE_MKDIR_ERR - errno, fmt::format(
                             "mkdir error for \"{}\", errno = \"{}\", ec = {}.",
                             subdir.c_str(), strerror(errno), ec));
            }
        }

        if ( pos == std::string::npos ) {
            done = true;
        }
    }

    return SUCCESS();
} // unix_file_mkdir_r

// =-=-=-=-=-=-=-
// 2. Define operations which will be called by the file*
//    calls declared in server/driver/include/fileDriver.h
// =-=-=-=-=-=-=-

// =-=-=-=-=-=-=-
// NOTE :: to access properties in the _prop_map do the
//      :: following :
//      :: double my_var = 0.0;
//      :: irods::error ret = _prop_map.get< double >( "my_key", my_var );
// =-=-=-=-=-=-=-

/// @brief interface to notify of a file registration
irods::error unix_file_registered(irods::plugin_context& _ctx)
{
    if (!is_operating_in_detached_mode(_ctx.prop_map())) {
        return SUCCESS();
    }

    irods::file_object_ptr fco = boost::dynamic_pointer_cast<irods::file_object>(_ctx.fco());
    std::string file_path = fco->physical_path();

    std::string vault_path;
    _ctx.prop_map().get<std::string>(irods::RESOURCE_PATH, vault_path);

    if (!has_prefix(file_path.c_str(), vault_path.c_str())) {
        // registered path not in vault, give warning
        std::string resource_name;
        _ctx.prop_map().get<std::string>(irods::RESOURCE_NAME, resource_name);

        auto warning_message = fmt::format(
            "Warning:  The registration was performed on a "
            "unixfilesystem resource [{}] currently configured as 'host_mode=detached'.  "
            "Verify that the path [{}] is available on all servers that may serve "
            "requests for this resource.",
            resource_name.c_str(),
            file_path.c_str());

        addRErrorMsg(&_ctx.comm()->rError, 0, warning_message.c_str());
    }

    return SUCCESS();
} // unix_file_registered

/// @brief interface to notify of a file unregistration
irods::error unix_file_unregistered(irods::plugin_context& _ctx)
{
    // NOOP
    return SUCCESS();
} // unix_file_unregistered

/// @brief interface to notify of a file modification
irods::error unix_file_modified(irods::plugin_context& _ctx)
{
    // NOOP
    return SUCCESS();
} // unix_file_modified

/// @brief interface to notify of a file operation
irods::error unix_file_notify(irods::plugin_context& _ctx, const std::string*)
{
    // NOOP
    return SUCCESS();
} // unix_file_notify

// interface to determine free space on a device given a path
irods::error unix_file_getfs_freespace(irods::plugin_context& _ctx)
{
    // Check the operation parameters and update the physical path
    if (const auto err = unix_check_params_and_path(_ctx); !err.ok()) {
        return PASSMSG("Invalid parameters or physical path.", err);
    }

    // cast down the hierarchy to the desired object
    irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
    size_t found = fco->physical_path().find_last_of( "/" );
    std::string path = fco->physical_path().substr( 0, found + 1 );
    int status = -1;
    rodsLong_t fssize = USER_NO_SUPPORT_ERR;
#if defined(solaris_platform)
    struct statvfs statbuf;
#else
    struct statfs statbuf;
#endif

    auto result = SUCCESS();
#if not defined(windows_platform)
#if defined(solaris_platform)
    status = statvfs( path.c_str(), &statbuf );
#elif defined(sgi_platform)
    status = statfs( path.c_str(), &statbuf, sizeof( struct statfs ), 0 );
#elif defined(aix_platform) || defined(linux_platform) || defined(osx_platform)
    status = statfs( path.c_str(), &statbuf );
#endif

    // handle error, if any
    if (status < 0) {
        const int err_status = UNIX_FILE_GET_FS_FREESPACE_ERR - errno;
        return ERROR(err_status, fmt::format(
                    "Statfs error for \"{}\", status = {}.",
                    path.c_str(), err_status));
    }

#if defined(sgi_platform)
    if ( statbuf.f_frsize > 0 ) {
        fssize = statbuf.f_frsize;
    }
    else {
        fssize = statbuf.f_bsize;
    }
    fssize *= statbuf.f_bavail;
#endif

#if defined(aix_platform) || defined(osx_platform) ||   \
    defined(linux_platform)
    fssize = statbuf.f_bavail * statbuf.f_bsize;
#elif defined(sgi_platform)
    fssize = statbuf.f_bfree * statbuf.f_bsize;
#endif
    result.code( fssize );

#endif /* solaris_platform, sgi_platform .... */

    return result;
} // unix_file_getfs_freespace

irods::error stat_vault_path(
    const std::string& _path,
    struct statfs&     _sb ) {
    namespace bfs = boost::filesystem;

    bfs::path vp( _path );

    // walk the vault path until we find an
    // existing directory which we can stat
    while( !bfs::exists( vp ) ) {
        vp = vp.parent_path();
    }

    if( vp.empty() ) {
        std::string msg( "path does not exist [" );
        msg += _path + "]";
        return ERROR(
                    SYS_INVALID_INPUT_PARAM,
                    msg );
    }

    int err = statfs( vp.string().c_str(), &_sb );
    if( err < 0 ) {
        std::stringstream msg;
        msg << "statfs failed for ["
            << _path << "] errno " << errno;
        return ERROR(
                errno,
                msg.str() );
    }

    return SUCCESS();

} // stat_vault_path

void warn_if_deprecated_context_string_set(
    irods::plugin_context& _ctx)
{
    std::string resource_name;
    irods::error ret = _ctx.prop_map().get<std::string>(irods::RESOURCE_NAME, resource_name);
    if (!ret.ok()) {
        rodsLog(LOG_ERROR,
            "%s: failed to get resource name",
            __FUNCTION__);
    }

    const std::vector<std::string> deprecated_keys{HIGH_WATER_MARK, REQUIRED_FREE_INODES_FOR_CREATE};

    for (const auto& k : deprecated_keys) {
        std::string holder{};
        irods::error result = _ctx.prop_map().get<std::string>(k, holder);
        if (KEY_NOT_FOUND != result.code()) {
            rodsLog(LOG_NOTICE,
                "%s: resource [%s] is using deprecated context string [%s]",
                __FUNCTION__,
                resource_name.c_str(),
                k.c_str());
        }
    }
} // warn_if_deprecated_context_string_set

// interface for POSIX create
irods::error unix_file_create(irods::plugin_context& _ctx)
{
    // Check the operation parameters and update the physical path
    if (const auto err = unix_check_params_and_path(_ctx); !err.ok()) {
        return PASSMSG("Invalid parameters or physical path.", err);
    }

    // get ref to fco
    irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

    char* kvp_str = getValByKey(
            &fco->cond_input(),
            KEY_VALUE_PASSTHROUGH_KW );
    if ( kvp_str ) {
        irods::kvp_map_t kvp;
        if (const auto err = irods::parse_kvp_string(kvp_str, kvp); !err.ok()) {
            irods::log(PASS(err));
        }
        else {
            irods::kvp_map_t::iterator itr = kvp.begin();
            for ( ; itr != kvp.end(); ++ itr ) {
                irods::experimental::log::resource::debug(
                        (boost::format("unix_file_create_plugin - kv_pass :: key [%s] - value [%s]") %
                         itr->first % itr->second).str());
            } // for itr
        }
    }

    const auto free_space_err = unix_file_getfs_freespace(_ctx);
    if (!free_space_err.ok()) {
        return PASSMSG("Error determining freespace on system.", free_space_err);
    }

    if (const rodsLong_t file_size = fco->size();
        file_size >= 0 && free_space_err.code() < file_size) {
        return ERROR(USER_FILE_TOO_LARGE, fmt::format(
                     "File size: {} is greater than space left on device: {}",
                     file_size, free_space_err.code()));
    }

    // make call to umask & open for create
    mode_t myMask = umask( ( mode_t ) 0000 );
    int    fd     = open( fco->physical_path().c_str(), O_RDWR | O_CREAT | O_EXCL, fco->mode() );
    int errsav = errno;

    // =-=-=-=-=-=-=-
    // reset the old mask
    ( void ) umask( ( mode_t ) myMask );

    // =-=-=-=-=-=-=-
    // if we got a 0 descriptor, try again
    if ( fd == 0 ) {
        close( fd );
        int null_fd = open( "/dev/null", O_RDWR, 0 );

        // =-=-=-=-=-=-=-
        // make call to umask & open for create
        mode_t myMask = umask( ( mode_t ) 0000 );
        fd = open( fco->physical_path().c_str(), O_RDWR | O_CREAT | O_EXCL, fco->mode() );
        errsav = errno;
        if ( null_fd >= 0 ) {
            close( null_fd );
        }
        rodsLog( LOG_NOTICE, "unix_file_create_plugin: 0 descriptor" );

        // =-=-=-=-=-=-=-
        // reset the old mask
        ( void ) umask( ( mode_t ) myMask );
    }

    // trap error case with bad fd
    if ( fd < 0 ) {
        int status = UNIX_FILE_CREATE_ERR - errsav;
        std::stringstream msg;
        msg << "create error for \"";
        msg << fco->physical_path();
        msg << "\", errno = \"";
        msg << strerror( errsav );
        msg << "\".";

        // WARNING :: Major Assumptions are made upstream and use the FD also as a
        //         :: Status, if this is not done EVERYTHING BREAKS!!!!111one
        fco->file_descriptor( status );

        const auto ret = ERROR(status, msg.str());
        irods::log(ret);
        return ret;
    }

    // cache file descriptor in out-variable
    fco->file_descriptor( fd );

    auto result = SUCCESS();
    result.code(fd);
    return result;
} // unix_file_create_plugin

// interface for POSIX Open
irods::error unix_file_open(irods::plugin_context& _ctx)
{
    // Check the operation parameters and update the physical path
    if (const auto err = unix_check_params_and_path(_ctx); !err.ok()) {
        return PASSMSG("Invalid parameters or physical path.", err);
    }

    // get ref to fco
    irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

    char* kvp_str = getValByKey(
            &fco->cond_input(),
            KEY_VALUE_PASSTHROUGH_KW );
    if ( kvp_str ) {
        irods::kvp_map_t kvp;
        if (const auto ret = irods::parse_kvp_string(kvp_str, kvp); !ret.ok()) {
            irods::log(PASS(ret));
        }
        else {
            irods::kvp_map_t::iterator itr = kvp.begin();
            for ( ; itr != kvp.end(); ++ itr ) {
                irods::experimental::log::resource::debug(
                        (boost::format("unix_file_open_plugin - kv_pass :: key [%s] - value [%s]") %
                         itr->first % itr->second).str());
            } // for itr
        }
    }

    // =-=-=-=-=-=-=-
    // handle OSX weirdness...
    int flags = fco->flags();

#if defined(osx_platform)
    // For osx, O_TRUNC = 0x0400, O_TRUNC = 0x200 for other system
    if ( flags & 0x200 ) {
        flags = flags ^ 0x200;
        flags = flags | O_TRUNC;
    }
#endif
    // =-=-=-=-=-=-=-
    // make call to open
    errno = 0;
    int fd = open( fco->physical_path().c_str(), flags, fco->mode() );
    int errsav = errno;

    // =-=-=-=-=-=-=-
    // if we got a 0 descriptor, try again
    if ( fd == 0 ) {
        close( fd );
        int null_fd = open( "/dev/null", O_RDWR, 0 );
        fd = open( fco->physical_path().c_str(), flags, fco->mode() );
        errsav = errno;
        if ( null_fd >= 0 ) {
            close( null_fd );
        }
        rodsLog( LOG_NOTICE, "unix_file_open: 0 descriptor" );
    }

    // =-=-=-=-=-=-=-
    // trap error case with bad fd
    if ( fd < 0 ) {
        int status = UNIX_FILE_OPEN_ERR - errsav;
        std::stringstream msg;
        msg << "Open error for \"";
        msg << fco->physical_path();
        msg << "\", errno = \"";
        msg << strerror( errsav );
        msg << "\", status = \"";
        msg << status;
        msg << "\", flags = \"";
        msg << flags;
        msg << "\".";
        return ERROR( status, msg.str() );
    }

    // cache status in the file object
    fco->file_descriptor( fd );

    auto result = SUCCESS();
    result.code(fd);
    return result;
} // unix_file_open

// interface for POSIX Read
irods::error unix_file_read(irods::plugin_context& _ctx,
                            void*                  _buf,
                            const int              _len)
{
    // Check the operation parameters and update the physical path
    if (const auto err = unix_check_params_and_path(_ctx); !err.ok()) {
        return PASSMSG("Invalid parameters or physical path.", err);
    }

    // get ref to fco
    irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

    // make the call to read
    const int status = read(fco->file_descriptor(), _buf, _len);

    // pass along an error if it was not successful
    if (status < 0) {
        const int err_status = UNIX_FILE_READ_ERR - errno;
        return ERROR(err_status, fmt::format(
                    "Read error for file: \"{}\", errno = \"{}\".",
                    fco->physical_path().c_str(), strerror(errno)));
    }

    auto result = SUCCESS();
    result.code(status);
    return result;
} // unix_file_read_plugin

// interface for POSIX Write
irods::error unix_file_write(irods::plugin_context& _ctx,
                             const void*            _buf,
                             const int              _len)
{
    // Check the operation parameters and update the physical path
    if (const auto err = unix_check_params_and_path(_ctx); !err.ok()) {
        return PASSMSG("Invalid parameters or physical path.", err);
    }

    // get ref to fco
    irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

    // make the call to write
    const int status = write(fco->file_descriptor(), _buf, _len);

    // pass along an error if it was not successful
    if (status < 0) {
        const int err_status = UNIX_FILE_WRITE_ERR - errno;
        return ERROR(err_status, fmt::format(
                    "Write file: \"{}\", errno = \"{}\", status = {}.",
                    fco->physical_path().c_str(), strerror(errno), err_status));
    }

    auto result = SUCCESS();
    result.code(status);
    return result;
} // unix_file_write_plugin

// interface for POSIX Close
irods::error unix_file_close(irods::plugin_context& _ctx)
{
    // Check the operation parameters and update the physical path
    if (const auto err = unix_check_params_and_path(_ctx); !err.ok()) {
        return PASSMSG("Invalid parameters or physical path.", err);
    }

    // get ref to fco
    irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

    // make the call to close
    const int status = close(fco->file_descriptor());

    // log any error
    if (status < 0) {
        int err_status = UNIX_FILE_CLOSE_ERR - errno;
        return ERROR(err_status, fmt::format(
                    "Close error for file: \"{}\", errno = \"{}\", status = {}.",
                    fco->physical_path().c_str(), strerror(errno), err_status));
    }

    auto result = SUCCESS();
    result.code(status);
    return result;
} // unix_file_close_plugin

// interface for POSIX Unlink
irods::error unix_file_unlink(irods::plugin_context& _ctx)
{
    // Check the operation parameters and update the physical path
    if (const auto err = unix_check_params_and_path(_ctx); !err.ok()) {
        return PASSMSG("Invalid parameters or physical path.", err);
    }

    // get ref to fco
    irods::data_object_ptr fco = boost::dynamic_pointer_cast< irods::data_object >( _ctx.fco() );

    // make the call to unlink
    const int status = unlink(fco->physical_path().c_str());

    // log any error
    if (status < 0) {
        int err_status = UNIX_FILE_UNLINK_ERR - errno;
        return ERROR(err_status, fmt::format(
                    "Unlink error for file: \"{}\", errno = \"{}\", status = {}.",
                    fco->physical_path().c_str(), strerror(errno), err_status));
    }

    auto result = SUCCESS();
    result.code(status);
    return result;
} // unix_file_unlink_plugin

// =-=-=-=-=-=-=-
// interface for POSIX Stat
irods::error unix_file_stat(irods::plugin_context& _ctx, struct stat* _statbuf)
{
    // NOTE:: this function assumes the object's physical path is
    //        correct and should not have the vault path
    //        prepended - hcj
    if (const auto err = _ctx.valid(); !err.ok()) {
        return PASSMSG("resource context is invalid.", err);
    }

    // get ref to fco
    irods::data_object_ptr fco = boost::dynamic_pointer_cast< irods::data_object >( _ctx.fco() );

    // make the call to stat
    const int status = stat( fco->physical_path().c_str(), _statbuf );

    // return an error if necessary
    if (status < 0) {
        const int err_status = UNIX_FILE_STAT_ERR - errno;
        return ERROR(err_status, fmt::format(
                    "Stat error for \"{}\", errno = \"{}\", status = {}.",
                    fco->physical_path().c_str(), strerror(errno), err_status));
    }

    auto result = SUCCESS();
    result.code(status);
    return result;
} // unix_file_stat_plugin

// interface for POSIX lseek
irods::error unix_file_lseek(irods::plugin_context& _ctx,
                             const long long        _offset,
                             const int              _whence)
{
    // Check the operation parameters and update the physical path
    if (const auto err = unix_check_params_and_path(_ctx); !err.ok()) {
        return PASSMSG("Invalid parameters or physical path.", err);
    }

    // get ref to fco
    irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

    // make the call to lseek
    const long long status = lseek(fco->file_descriptor(), _offset, _whence);

    // log any error
    if (status < 0) {
        const long long err_status = UNIX_FILE_LSEEK_ERR - errno;
        return ERROR(err_status, fmt::format(
                    "Lseek error for \"{}\", errno = \"{}\", status = {}.",
                    fco->physical_path().c_str(), strerror(errno), err_status));
    }

    auto result = SUCCESS();
    result.code(status);
    return result;
} // unix_file_lseek_plugin

// interface for POSIX mkdir
irods::error unix_file_mkdir(irods::plugin_context& _ctx)
{
    // NOTE :: this function assumes the object's physical path is correct and
    //         should not have the vault path prepended - hcj
    if (const auto err = _ctx.valid(); !err.ok()) {
        return PASSMSG("resource context is invalid.", err);
    }

    // cast down the chain to our understood object type
    irods::collection_object_ptr fco = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );

    // make the call to mkdir & umask
    mode_t myMask = umask( ( mode_t ) 0000 );
    const int status = mkdir( fco->physical_path().c_str(), fco->mode() );

    // reset the old mask
    umask( ( mode_t ) myMask );

    // return an error if necessary
    if (status < 0) {
        const int err_status = UNIX_FILE_MKDIR_ERR - errno;
        return ERROR(err_status, fmt::format(
                     "Mkdir error for \"{}\", errno = \"{}\", status = {}.",
                     fco->physical_path().c_str(), strerror(errno), err_status));
    }

    auto result = SUCCESS();
    result.code(status);
    return result;
} // unix_file_mkdir_plugin

// interface for POSIX rmdir
irods::error unix_file_rmdir(irods::plugin_context& _ctx)
{
    // Check the operation parameters and update the physical path
    if (const auto err = unix_check_params_and_path(_ctx); !err.ok()) {
        return PASSMSG("Invalid parameters or physical path.", err);
    }

    // cast down the chain to our understood object type
    irods::collection_object_ptr fco = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );

    // make the call to rmdir
    const int status = rmdir( fco->physical_path().c_str() );

    // return an error if necessary
    if (status < 0) {
        const int err_status = UNIX_FILE_RMDIR_ERR - errno;
        return ERROR(err_status, fmt::format(
                     "Rmdir error for \"{}\", errno = \"{}\", status = {}.",
                     fco->physical_path().c_str(), strerror(errno), err_status));
    }

    return SUCCESS();
} // unix_file_rmdir_plugin

// interface for POSIX opendir
irods::error unix_file_opendir(irods::plugin_context& _ctx)
{
    // Check the operation parameters and update the physical path
    if (const auto ret = unix_check_params_and_path<irods::collection_object>(_ctx); !ret.ok()) {
        return PASSMSG("Invalid parameters or physical path.", ret);
    }

    // cast down the chain to our understood object type
    irods::collection_object_ptr fco = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );

    // make the call to opendir
    DIR* dir_ptr = opendir( fco->physical_path().c_str() );

    // trap error case with bad fd
    if (!dir_ptr) {
        const int errsav = errno;

        const int status = UNIX_FILE_CREATE_ERR - errsav;
        std::stringstream msg;
        msg << "Open error for \"";
        msg << fco->physical_path();
        msg << "\", errno = \"";
        msg << strerror( errsav );
        msg << "\", status = \"";
        msg << status;
        msg << "\".";
        return ERROR(status, msg.str());
    }

    // cache dir_ptr in the out-variable
    fco->directory_pointer( dir_ptr );

    return SUCCESS();
} // unix_file_opendir_plugin

// interface for POSIX closedir
irods::error unix_file_closedir(irods::plugin_context& _ctx)
{
    // Check the operation parameters and update the physical path
    if (const auto ret = unix_check_params_and_path<irods::collection_object>(_ctx); !ret.ok()) {
        return PASSMSG("Invalid parameters or physical path.", ret);
    }

    // cast down the chain to our understood object type
    irods::collection_object_ptr fco = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );

    // make the call to opendir
    const int status = closedir(fco->directory_pointer());

    // return an error if necessary
    if (status < 0) {
        int err_status = UNIX_FILE_CLOSEDIR_ERR - errno;
        return ERROR(err_status, fmt::format(
                    "Closedir error for \"{}\", errno = \"{}\", status = {}.",
                    fco->physical_path().c_str(), strerror(errno), err_status));
    }

    return SUCCESS();
} // unix_file_closedir_plugin

// interface for POSIX readdir
irods::error unix_file_readdir(irods::plugin_context& _ctx, struct rodsDirent** _dirent_ptr)
{
    // Check the operation parameters and update the physical path
    if (const auto ret = unix_check_params_and_path<irods::collection_object>(_ctx); !ret.ok()) {
        return PASSMSG("Invalid parameters or physical path.", ret);
    }

    // cast down the chain to our understood object type
    irods::collection_object_ptr fco = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );

    // zero out errno?
    errno = 0;

    // make the call to readdir
    struct dirent * tmp_dirent = readdir(fco->directory_pointer());

    // handle error cases
    if (!tmp_dirent) {
        // cache status in out variable
        const int status = UNIX_FILE_READDIR_ERR - errno;
        if (errno != 0) {
            return ERROR(status, fmt::format(
                        "Readdir error, status = {}, errno= \"{}\".",
                        status, strerror(errno)));
        }

        return ERROR(-1, "End of directory list reached.");
    }

    // alloc dirent as necessary
    if (!(*_dirent_ptr)) {
        *_dirent_ptr = static_cast<rodsDirent_t*>(malloc(sizeof(rodsDirent_t)));
        memset(*_dirent_ptr, 0, sizeof(rodsDirent_t));
    }

    // convert standard dirent to rods dirent struct
    if (const auto ec = direntToRodsDirent(*_dirent_ptr, tmp_dirent); ec < 0) {
        irods::log(ERROR(ec, "direntToRodsDirent failed."));
    }

#if defined(solaris_platform)
    rstrcpy((*_dirent_ptr)->d_name, tmp_dirent->d_name, MAX_NAME_LEN);
#endif

    return SUCCESS();
} // unix_file_readdir_plugin

// interface for POSIX readdir
irods::error unix_file_rename(irods::plugin_context& _ctx, const char* _new_file_name)
{
    // Check the operation parameters and update the physical path
    if (const auto ret = unix_check_params_and_path(_ctx); !ret.ok()) {
        return PASSMSG("Invalid parameters or physical path.", ret);
    }

    // manufacture a new path from the new file name
    std::string new_full_path;
    if (const auto err = unix_generate_full_path(_ctx.prop_map(), _new_file_name, new_full_path); !err.ok()) {
        return PASSMSG(fmt::format(
                    "Unable to generate full path for destination file: \"{}\".",
                    _new_file_name), err);
    }

    // cast down the hierarchy to the desired object
    irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

    // get the default directory mode
    mode_t mode = 0750;
    if (const auto err = _ctx.prop_map().get<mode_t>(DEFAULT_VAULT_DIR_MODE, mode); !err.ok()) {
        return PASS(err);
    }

    // make the directories in the path to the new file
    std::string new_path = new_full_path;
    std::size_t last_slash = new_path.find_last_of( '/' );
    new_path.erase( last_slash );
    if (const auto err = unix_file_mkdir_r(new_path, mode); !err.ok()) {
        irods::log(LOG_DEBUG, err.result());
    }

    // make the call to rename
    const int status = rename(fco->physical_path().c_str(), new_full_path.c_str());

    // issue 4326 - plugins must set the physical path to the new path
    fco->physical_path(new_full_path);

    // handle error cases
    if (status < 0) {
        const int err_status = UNIX_FILE_RENAME_ERR - errno;
        return ERROR(err_status, fmt::format(
                    "Rename error for \"{}\" to \"{}\", errno = \"{}\", status = {}.",
                    fco->physical_path().c_str(), new_full_path.c_str(), strerror(errno), err_status));
    }

    auto result = SUCCESS();
    result.code(status);
    return result;
} // unix_file_rename_plugin

// interface for POSIX truncate
irods::error unix_file_truncate(irods::plugin_context& _ctx)
{
    // Check the operation parameters and update the physical path
    if (const auto ret = unix_check_params_and_path<irods::file_object>(_ctx); !ret.ok()) {
        return PASSMSG("Invalid parameters or physical path.", ret);
    }

    // cast down the chain to our understood object type
    irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

    // make the call to rename
    const int status = truncate(file_obj->physical_path().c_str(), file_obj->size());

    // handle any error cases
    if (status < 0) {
        const int err_status = UNIX_FILE_TRUNCATE_ERR - errno;
        return ERROR(err_status, fmt::format(
                    "Truncate error for: \"{}\", errno = \"{}\", status = {}.",
                    file_obj->physical_path().c_str(), strerror(errno), err_status));
    }

    return SUCCESS();
} // unix_file_truncate_plugin


// unixStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
// Just copy the file from filename to cacheFilename. optionalInfo info
// is not used.
irods::error unix_file_stage_to_cache(irods::plugin_context& _ctx, const char* _cache_file_name)
{
    // Check the operation parameters and update the physical path
    if (const auto ret = unix_check_params_and_path(_ctx); !ret.ok()) {
        return PASSMSG("Invalid parameters or physical path.", ret);
    }

    // cast down the hierarchy to the desired object
    irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

    if (const auto err = unix_file_copy(fco->mode(), fco->physical_path().c_str(), _cache_file_name); !err.ok()) {
        return err;
    }

    return SUCCESS();
} // unix_file_stagetocache

// unixSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
// Just copy the file from cacheFilename to filename. optionalInfo info
// is not used.
irods::error unix_file_sync_to_arch(irods::plugin_context& _ctx, const char* _cache_file_name)
{
    // Check the operation parameters and update the physical path
    if (const auto ret = unix_check_params_and_path(_ctx); !ret.ok()) {
        return PASSMSG("Invalid parameters or physical path.", ret);
    }

    // cast down the hierarchy to the desired object
    irods::file_object_ptr fco = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

    if (const auto err = unix_file_copy(fco->mode(), _cache_file_name, fco->physical_path().c_str()); !err.ok()) {
        return err;
    }

    return SUCCESS();
} // unix_file_sync_to_arch

// used to allow the resource to determine which host
// should provide the requested operation
irods::error unix_file_resolve_hierarchy(
    irods::plugin_context&   _ctx,
    const std::string*       _opr,
    const std::string*       _curr_host,
    irods::hierarchy_parser* _out_parser,
    float*                   _out_vote)
{
    namespace irv = irods::experimental::resource::voting;

    if (irods::error ret = _ctx.valid<irods::file_object>(); !ret.ok()) {
        return PASSMSG("Invalid resource context.", ret);
    }

    if (!_opr || !_curr_host || !_out_parser || !_out_vote) {
        return ERROR(SYS_INVALID_INPUT_PARAM, "Invalid input parameter.");
    }

    if (irods::CREATE_OPERATION == *_opr) {
        warn_if_deprecated_context_string_set(_ctx);
    }

    irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object>(_ctx.fco());
    if (getValByKey(&file_obj->cond_input(), RECURSIVE_OPR__KW)) {
        irods::experimental::log::resource::debug(
            (boost::format("%s: %s found in cond_input for file_obj") %
            __FUNCTION__ % RECURSIVE_OPR__KW).str());
    }

    _out_parser->add_child(irods::get_resource_name(_ctx));
    *_out_vote = irv::vote::zero;
    try {
        *_out_vote = irv::calculate(*_opr, _ctx, *_curr_host, *_out_parser);
        return SUCCESS();
    }
    catch(const std::out_of_range& e) {
        return ERROR(INVALID_OPERATION, e.what());
    }
    catch (const irods::exception& e) {
        return irods::error(e);
    }
    return ERROR(SYS_UNKNOWN_ERROR, "An unknown error occurred while resolving hierarchy.");
} // unix_file_resolve_hierarchy

// =-=-=-=-=-=-=-
// unix_file_rebalance - code which would rebalance the subtree
irods::error unix_file_rebalance(
    irods::plugin_context& _ctx ) {
    return SUCCESS();

} // unix_file_rebalance

// =-=-=-=-=-=-=-
// 3. create derived class to handle unix file system resources
//    necessary to do custom parsing of the context string to place
//    any useful values into the property map for reference in later
//    operations.  semicolon is the preferred delimiter
class unixfilesystem_resource : public irods::resource {
        // =-=-=-=-=-=-=-
        // 3a. create a class to provide maintenance operations, this is only for example
        //     and will not be called.
        class maintenance_operation {
            public:
                maintenance_operation( const std::string& _n ) : name_( _n ) {
                }

                maintenance_operation( const maintenance_operation& _rhs ): name_(_rhs.name_) {
                }

                maintenance_operation& operator=( const maintenance_operation& _rhs ) {
                    name_ = _rhs.name_;
                    return *this;
                }

                irods::error operator()( rcComm_t* ) {
                    rodsLog( LOG_NOTICE, "unixfilesystem_resource::post_disconnect_maintenance_operation - [%s]",
                             name_.c_str() );
                    return SUCCESS();
                }

            private:
                std::string name_;

        }; // class maintenance_operation

    public:
        unixfilesystem_resource(
            const std::string& _inst_name,
            const std::string& _context ) :
            irods::resource(
                _inst_name,
                _context ) {
            properties_.set<mode_t>( DEFAULT_VAULT_DIR_MODE, 0750 );
            // =-=-=-=-=-=-=-
            // parse context string into property pairs assuming a ; as a separator
            std::vector< std::string > props;
            irods::kvp_map_t kvp;
            irods::parse_kvp_string(
                _context,
                kvp );

                // =-=-=-=-=-=-=-
                // copy the properties from the context to the prop map
                irods::kvp_map_t::iterator itr = kvp.begin();
                for( ; itr != kvp.end(); ++itr ) {
                    properties_.set< std::string >(
                        itr->first,
                        itr->second );
                } // for itr

            set_start_operation(unix_file_start_operation);

        } // ctor

        irods::error need_post_disconnect_maintenance_operation( bool& _b ) {
            _b = false;
            return SUCCESS();
        }

        // =-=-=-=-=-=-=-
        // 3b. pass along a functor for maintenance work after
        //     the client disconnects, uncomment the first two lines for effect.
        irods::error post_disconnect_maintenance_operation( irods::pdmo_type& ) {
            irods::error result = SUCCESS();
            return ERROR( -1, "nop" );
        }
}; // class unixfilesystem_resource

// =-=-=-=-=-=-=-
// 4. create the plugin factory function which will return a dynamically
//    instantiated object of the previously defined derived resource.  use
//    the add_operation member to associate a 'call name' to the interfaces
//    defined above.  for resource plugins these call names are standardized
//    as used by the irods facing interface defined in
//    server/drivers/src/fileDriver.c
extern "C"
irods::resource* plugin_factory( const std::string& _inst_name, const std::string& _context ) {

    // =-=-=-=-=-=-=-
    // 4a. create unixfilesystem_resource
    unixfilesystem_resource* resc = new unixfilesystem_resource( _inst_name, _context );

    // =-=-=-=-=-=-=-
    // 4b. map function names to operations.  this map will be used to load
    //     the symbols from the shared object in the delay_load stage of
    //     plugin loading.
    using namespace irods;
    using namespace std;
    resc->add_operation(
        RESOURCE_OP_CREATE,
        function<error(plugin_context&)>(
            unix_file_create ) );

    resc->add_operation(
        irods::RESOURCE_OP_OPEN,
        function<error(plugin_context&)>(
            unix_file_open ) );

    resc->add_operation(
        irods::RESOURCE_OP_READ,
        std::function<
            error(irods::plugin_context&,void*,const int)>(
                unix_file_read ) );

    resc->add_operation(
        irods::RESOURCE_OP_WRITE,
        function<error(plugin_context&,const void*,const int)>(
            unix_file_write ) );

    resc->add_operation(
        RESOURCE_OP_CLOSE,
        function<error(plugin_context&)>(
            unix_file_close ) );

    resc->add_operation(
        irods::RESOURCE_OP_UNLINK,
        function<error(plugin_context&)>(
            unix_file_unlink ) );

    resc->add_operation(
        irods::RESOURCE_OP_STAT,
        function<error(plugin_context&, struct stat*)>(
            unix_file_stat ) );

    resc->add_operation(
        irods::RESOURCE_OP_MKDIR,
        function<error(plugin_context&)>(
            unix_file_mkdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_OPENDIR,
        function<error(plugin_context&)>(
            unix_file_opendir ) );

    resc->add_operation(
        irods::RESOURCE_OP_READDIR,
        function<error(plugin_context&,struct rodsDirent**)>(
            unix_file_readdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_RENAME,
        function<error(plugin_context&, const char*)>(
            unix_file_rename ) );

    resc->add_operation(
        irods::RESOURCE_OP_FREESPACE,
        function<error(plugin_context&)>(
            unix_file_getfs_freespace ) );

    resc->add_operation(
        irods::RESOURCE_OP_LSEEK,
        function<error(plugin_context&, const long long, const int)>(
            unix_file_lseek ) );

    resc->add_operation(
        irods::RESOURCE_OP_RMDIR,
        function<error(plugin_context&)>(
            unix_file_rmdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_CLOSEDIR,
        function<error(plugin_context&)>(
            unix_file_closedir ) );

    resc->add_operation(
        irods::RESOURCE_OP_STAGETOCACHE,
        function<error(plugin_context&, const char*)>(
            unix_file_stage_to_cache ) );

    resc->add_operation(
        irods::RESOURCE_OP_SYNCTOARCH,
        function<error(plugin_context&, const char*)>(
            unix_file_sync_to_arch ) );

    resc->add_operation(
        irods::RESOURCE_OP_REGISTERED,
        function<error(plugin_context&)>(
            unix_file_registered ) );

    resc->add_operation(
        irods::RESOURCE_OP_UNREGISTERED,
        function<error(plugin_context&)>(
            unix_file_unregistered ) );

    resc->add_operation(
        irods::RESOURCE_OP_MODIFIED,
        function<error(plugin_context&)>(
            unix_file_modified ) );

    resc->add_operation(
        irods::RESOURCE_OP_NOTIFY,
        function<error(plugin_context&, const std::string*)>(
            unix_file_notify ) );

    resc->add_operation(
        irods::RESOURCE_OP_TRUNCATE,
        function<error(plugin_context&)>(
            unix_file_truncate ) );

    resc->add_operation(
        irods::RESOURCE_OP_RESOLVE_RESC_HIER,
        function<error(plugin_context&,const std::string*, const std::string*, irods::hierarchy_parser*, float*)>(
            unix_file_resolve_hierarchy ) );

    resc->add_operation(
        irods::RESOURCE_OP_REBALANCE,
        function<error(plugin_context&)>(
            unix_file_rebalance ) );

    // =-=-=-=-=-=-=-
    // set some properties necessary for backporting to iRODS legacy code
    resc->set_property< int >( irods::RESOURCE_CHECK_PATH_PERM, 2 );//DO_CHK_PATH_PERM );
    resc->set_property< int >( irods::RESOURCE_CREATE_PATH,     1 );//CREATE_PATH );

    // =-=-=-=-=-=-=-
    // 4c. return the pointer through the generic interface of an
    //     irods::resource pointer
    return dynamic_cast<irods::resource*>( resc );

} // plugin_factory
