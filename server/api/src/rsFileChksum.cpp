#include "irods/rsFileChksum.hpp"

#include "irods/miscServerFunct.hpp"
#include "irods/rodsErrorTable.h"
#include "irods/irods_log.hpp"
#include "irods/irods_file_object.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/irods_resource_backport.hpp"
#include "irods/irods_hasher_factory.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/MD5Strategy.hpp"
#include "irods/irods_at_scope_exit.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <string>
#include <string_view>

#define SVR_MD5_BUF_SZ (1024*1024)

int rsFileChksum(rsComm_t* rsComm, fileChksumInp_t* fileChksumInp, char** chksumStr)
{
    rodsServerHost_t* rodsServerHost;
    int remoteFlag;
    irods::error ret = irods::get_host_for_hier_string(fileChksumInp->rescHier, remoteFlag, rodsServerHost);
    if (!ret.ok()) {
        irods::log(PASSMSG("failed in call to irods::get_host_for_hier_string", ret));
        return -1;
    }

    if (LOCAL_HOST == remoteFlag) {
        return _rsFileChksum(rsComm, fileChksumInp, chksumStr);
    }

    if (REMOTE_HOST == remoteFlag) {
        return remoteFileChksum(rsComm, fileChksumInp, chksumStr, rodsServerHost);
    }

    if (remoteFlag < 0) {
        return remoteFlag;
    }

    rodsLog(LOG_NOTICE, "rsFileChksum: resolveHost returned unrecognized value %d", remoteFlag);

    return SYS_UNRECOGNIZED_REMOTE_FLAG;
} // rsFileChksum

int remoteFileChksum(rsComm_t* rsComm,
                     fileChksumInp_t* fileChksumInp,
                     char** chksumStr,
                     rodsServerHost_t* rodsServerHost)
{
    if (!rodsServerHost) {
        rodsLog(LOG_NOTICE, "remoteFileChksum: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if (const auto ec = svrToSvrConnect(rsComm, rodsServerHost); ec < 0) {
        return ec;
    }

    const auto status = rcFileChksum(rodsServerHost->conn, fileChksumInp, chksumStr);

    if (status < 0) {
        rodsLog(LOG_NOTICE,
                "remoteFileChksum: rcFileChksum failed for %s",
                fileChksumInp->fileName);
    }

    return status;
} // remoteFileChksum

int _rsFileChksum(rsComm_t* rsComm, fileChksumInp_t* fileChksumInp, char** chksumStr)
{
    if (!*chksumStr) {
        *chksumStr = static_cast<char*>(std::malloc(sizeof(char) * NAME_LEN));
    }

    const auto ec = file_checksum(rsComm,
                                  fileChksumInp->objPath,
                                  fileChksumInp->fileName,
                                  fileChksumInp->rescHier,
                                  fileChksumInp->orig_chksum,
                                  fileChksumInp->dataSize,
                                  *chksumStr);
    if (ec < 0) {
        rodsLog(LOG_DEBUG, "%s :: file_checksum for %s, status = %d",
                __func__, fileChksumInp->fileName, ec);
        std::free(*chksumStr);
        *chksumStr = nullptr;
    }

    return ec;
} // _rsFileChksum

int fileChksum(rsComm_t* rsComm,
               char* objPath,
               char* fileName,
               char* rescHier,
               char* orig_chksum,
               char* chksumStr)
{
    // =-=-=-=-=-=-=-
    // capture server hashing settings
    std::string hash_scheme( irods::MD5_NAME );
    try {
        hash_scheme = irods::get_server_property<const std::string>(irods::KW_CFG_DEFAULT_HASH_SCHEME);
    } catch ( const irods::exception& ) {}

    // make sure the read parameter is lowercased
    std::transform(
        hash_scheme.begin(),
        hash_scheme.end(),
        hash_scheme.begin(),
        ::tolower );

    std::string hash_policy;
    try {
        hash_policy = irods::get_server_property<const std::string>(irods::KW_CFG_MATCH_HASH_POLICY);
    } catch ( const irods::exception& ) {}

    // =-=-=-=-=-=-=-
    // extract scheme from checksum string
    std::string chkstr_scheme;
    if ( orig_chksum ) {
        irods::error ret = irods::get_hash_scheme_from_checksum(
                  orig_chksum,
                  chkstr_scheme );
        if ( !ret.ok() ) {
            //irods::log( PASS( ret ) );
        }
    }

    // =-=-=-=-=-=-=-
    // check the hash scheme against the policy
    // if necessary
    std::string final_scheme( hash_scheme );
    if ( !chkstr_scheme.empty() ) {
        if ( !hash_policy.empty() ) {
            if ( irods::STRICT_HASH_POLICY == hash_policy ) {
                if ( hash_scheme != chkstr_scheme ) {
                    return USER_HASH_TYPE_MISMATCH;
                }
            }
        }
        final_scheme = chkstr_scheme;
    }

    rodsLog(
        LOG_DEBUG,
        "fileChksum :: final_scheme [%s]  chkstr_scheme [%s]  hash_policy [%s]",
        final_scheme.c_str(),
        chkstr_scheme.c_str(),
        hash_policy.c_str() );

    // =-=-=-=-=-=-=-
    // call resource plugin to open file
    irods::file_object_ptr file_obj(
        new irods::file_object(
            rsComm,
            objPath,
            fileName,
            rescHier,
            -1, 0, O_RDONLY ) ); // FIXME :: hack until this is better abstracted - JMC
    irods::error ret = fileOpen( rsComm, file_obj );
    if ( !ret.ok() ) {
        int status = UNIX_FILE_OPEN_ERR - errno;
        if ( ret.code() != DIRECT_ARCHIVE_ACCESS ) {
            std::stringstream msg;
            msg << "fileOpen failed for [";
            msg << fileName;
            msg << "]";
            irods::log( PASSMSG( msg.str(), ret ) );
        }
        else {
            status = ret.code();
        }
        return status;
    }

    // =-=-=-=-=-=-=-
    // create a hasher object and init given a scheme
    // if it is unsupported then default to md5
    irods::Hasher hasher;
    ret = irods::getHasher( final_scheme, hasher );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        irods::getHasher( irods::MD5_NAME, hasher );
    }

    // =-=-=-=-=-=-=-
    // do an initial read of the file
    char buffer[SVR_MD5_BUF_SZ];
    irods::error read_err = fileRead(
                                rsComm,
                                file_obj,
                                buffer,
                                SVR_MD5_BUF_SZ );
    if (!read_err.ok()) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Failed to read buffer from file: \"";
        msg << fileName;
        msg << "\"";
        irods::error result = PASSMSG( msg.str(), read_err );
        irods::log( result );
        return result.code();
    }
    int bytes_read = read_err.code();
    
    // RTS - Issue #3275
    if ( bytes_read == 0 ) {
        std::string buffer_read;
        buffer_read.resize( SVR_MD5_BUF_SZ );
    }

    // =-=-=-=-=-=-=-
    // loop and update while there are still bytes to be read
    while ( read_err.ok() && bytes_read > 0 ) {
        // =-=-=-=-=-=-=-
        // update hasher
        hasher.update( std::string( buffer, bytes_read ) );

        // =-=-=-=-=-=-=-
        // read some more
        read_err = fileRead( rsComm, file_obj, buffer, SVR_MD5_BUF_SZ );
        if ( read_err.ok() ) {
            bytes_read = read_err.code();
        }
        else {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to read buffer from file: \"";
            msg << fileName;
            msg << "\"";
            irods::error result = PASSMSG( msg.str(), read_err );
            irods::log( result );
            return result.code();
        }

    } // while

    // =-=-=-=-=-=-=-
    // close out the file
    ret = fileClose( rsComm, file_obj );
    if ( !ret.ok() ) {
        irods::error err = PASSMSG( "error on close", ret );
        irods::log( err );
    }

    // =-=-=-=-=-=-=-
    // extract the digest from the hasher object
    // and copy to outgoing string
    std::string digest;
    hasher.digest( digest );
    strncpy( chksumStr, digest.c_str(), NAME_LEN );

    return 0;
} // fileChksum

int file_checksum(RsComm* _comm,
                  const char* _logical_path,
                  const char* _filename,
                  const char* _resource_hierarchy,
                  const char* _original_checksum,
                  rodsLong_t _data_size,
                  char* _calculated_checksum)
{
    // Capture server hashing settings.
    std::string hash_scheme = irods::MD5_NAME;
    try {
        hash_scheme = irods::get_server_property<const std::string>(irods::KW_CFG_DEFAULT_HASH_SCHEME);
    }
    catch (const irods::exception&) {}

    // Make sure the read parameter is lowercased.
    std::transform(hash_scheme.begin(), hash_scheme.end(), hash_scheme.begin(), ::tolower);

    std::string_view hash_policy;
    try {
        hash_policy = irods::get_server_property<const std::string>(irods::KW_CFG_MATCH_HASH_POLICY);
    }
    catch (const irods::exception&) {}

    // Extract scheme from checksum string.
    std::string chkstr_scheme;
    if (_original_checksum) {
        irods::get_hash_scheme_from_checksum(_original_checksum, chkstr_scheme);
    }

    // Check the hash scheme against the policy if necessary.
    std::string_view final_scheme = hash_scheme;
    if (!chkstr_scheme.empty()) {
        if (!hash_policy.empty()) {
            if (irods::STRICT_HASH_POLICY == hash_policy) {
                if (hash_scheme != chkstr_scheme) {
                    return USER_HASH_TYPE_MISMATCH;
                }
            }
        }

        final_scheme = chkstr_scheme;
    }

    rodsLog(LOG_DEBUG, "file_checksum :: final_scheme [%s]  chkstr_scheme [%s]  hash_policy [%s]",
            final_scheme.data(), chkstr_scheme.c_str(), hash_policy.data());

    // Create a hasher object and init given a scheme if it is unsupported then default to md5.
    irods::Hasher hasher;
    if (const auto error = irods::getHasher(final_scheme.data(), hasher); !error.ok()) {
        irods::log(PASS(error));
        irods::getHasher(irods::MD5_NAME, hasher);
    }

    irods::hierarchy_parser hp{_resource_hierarchy};
    std::string leaf_resc;

    if (const auto error = hp.last_resc(leaf_resc); !error.ok()) {
        return error.code();
    }

    irods::file_object_ptr file_ptr{new irods::file_object{
        _comm, _logical_path, _filename, _resource_hierarchy, -1, 0, O_RDONLY}};

    if (const auto error = fileOpen(_comm, file_ptr); !error.ok()) {
        if (const auto ec = UNIX_FILE_OPEN_ERR - errno; error.code() != DIRECT_ARCHIVE_ACCESS) {
            irods::log(PASSMSG(fmt::format("fileOpen failed for [{}].", _filename), error));
            return ec;
        }

        return error.code();
    }

    irods::at_scope_exit close_replica{[_comm, file_ptr, &_filename] {
        if (const auto error = fileClose(_comm, file_ptr); !error.ok()) {
            irods::log(PASSMSG(fmt::format("{} - fileClose error for [{}].", __func__, _filename), error));
        }
    }};

    char buffer[SVR_MD5_BUF_SZ];
    irods::error error = SUCCESS();

    while (_data_size > 0) {
        error = fileRead(_comm, file_ptr, buffer, std::min<std::int64_t>(_data_size, sizeof(buffer)));

        if (error.code() <= 0) {
            const auto msg = fmt::format("{} - fileRead failed for [{}].", __func__, _filename);
            irods::log(PASSMSG(msg, error));

            if (error.code() == 0 && _data_size > 0) {
                rodsLog(LOG_ERROR, "%s - The size of the replica recorded in the catalog is greater "
                                   "than the size in storage.", __func__);
                return UNIX_FILE_READ_ERR;
            }

            return error.code();
        }

        _data_size -= error.code();
        hasher.update(std::string(buffer, error.code()));
    }

    // extract the digest from the hasher object
    // and copy to outgoing string
    std::string digest;
    hasher.digest(digest);
    strncpy(_calculated_checksum, digest.c_str(), NAME_LEN);

    return 0;
} // file_checksum

