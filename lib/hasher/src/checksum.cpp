/* md5Checksum.c - checksumming routine on the client side
 */

#include "irods_stacktrace.hpp"
#include "irods_hasher_factory.hpp"
#include "getRodsEnv.h"
#include "irods_log.hpp"
#include "objInfo.h"
#include "SHA256Strategy.hpp"
#include "rodsKeyWdDef.h"
#include "rcMisc.h"
#include "checksum.hpp"

#include <fstream>

#define HASH_BUF_SZ (1024*1024)

int chksumLocFile(
    const char*       _file_name,
    char*       _checksum,
    const char* _hash_scheme ) {
    if ( !_file_name ||
            !_checksum  ||
            !_hash_scheme ) {
        rodsLog(
            LOG_ERROR,
            "chksumLocFile :: null input param - %p %p %p",
            _file_name,
            _checksum,
            _hash_scheme );
        return SYS_INVALID_INPUT_PARAM;
    }

    // =-=-=-=-=-=-=-
    // capture client side configuration
    rodsEnv env;
    int status = getRodsEnv( &env );
    if ( status < 0 ) {
        return status;
    }

    // =-=-=-=-=-=-=-
    // capture the configured scheme if it is valid
    std::string env_scheme( irods::SHA256_NAME );
    if ( strlen( env.rodsDefaultHashScheme ) > 0 ) {
        env_scheme = env.rodsDefaultHashScheme;

    }

    // =-=-=-=-=-=-=-
    // capture the configured hash match policy if it is valid
    std::string env_policy;
    if ( strlen( env.rodsMatchHashPolicy ) > 0 ) {
        env_policy = env.rodsMatchHashPolicy;
        // =-=-=-=-=-=-=-
        // hash scheme keywords are all lowercase
        std::transform(
            env_scheme.begin(),
            env_scheme.end(),
            env_scheme.begin(),
            ::tolower );
    }

    // =-=-=-=-=-=-=-
    // capture the incoming scheme if it is valid
    std::string hash_scheme;
    if ( _hash_scheme &&
            strlen( _hash_scheme ) > 0 &&
            strlen( _hash_scheme ) < NAME_LEN ) {
        hash_scheme = _hash_scheme;
        // =-=-=-=-=-=-=-
        // hash scheme keywords are all lowercase
        std::transform(
            hash_scheme.begin(),
            hash_scheme.end(),
            hash_scheme.begin(),
            ::tolower );
    }
    else {
        hash_scheme = env_scheme;

    }

    // =-=-=-=-=-=-=-
    // hash scheme keywords are all lowercase
    std::transform(
        hash_scheme.begin(),
        hash_scheme.end(),
        hash_scheme.begin(),
        ::tolower );
    std::transform(
        env_scheme.begin(),
        env_scheme.end(),
        env_scheme.begin(),
        ::tolower );


    // =-=-=-=-=-=-=-
    // verify checksum scheme against configuration
    // if requested
    std::string final_scheme( env_scheme );
    if ( !hash_scheme.empty() ) {
        if ( !env_policy.empty() ) {
            if ( irods::STRICT_HASH_POLICY == env_policy ) {
                if ( env_scheme != hash_scheme ) {
                    return USER_HASH_TYPE_MISMATCH;
                }
            }
        }

        final_scheme = hash_scheme;
    }

    // =-=-=-=-=-=-=-
    // init the hasher object
    irods::Hasher hasher;
    irods::error ret = irods::getHasher(
                           final_scheme,
                           hasher );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();

    }

    // =-=-=-=-=-=-=-
    // open the local file
    std::ifstream in_file(
        _file_name,
        std::ios::in | std::ios::binary );
    if ( !in_file.is_open() ) {
        status = UNIX_FILE_OPEN_ERR - errno;
        rodsLogError(
            LOG_ERROR,
            status,
            "chksumLocFile - fopen failed for %s, status = %d",
            _file_name,
            status );
        return status;
    }

    std::string buffer_read;
    buffer_read.resize( HASH_BUF_SZ );

    while ( in_file.read( &buffer_read[0], HASH_BUF_SZ ) ) {
        hasher.update( buffer_read );
    }

    if ( in_file.eof() ) {
        if ( in_file.gcount() > 0 ) {
            buffer_read.resize( in_file.gcount() );
            hasher.update( buffer_read );
        }
    } else {
        status = UNIX_FILE_READ_ERR - errno;
        rodsLogError(
                     LOG_ERROR,
                     status,
                     "chksumLocFile - read() failed for %s, status = %d",
                     _file_name,
                     status );
        return status;
    }

    // =-=-=-=-=-=-=-
    // capture the digest
    std::string digest;
    hasher.digest( digest );
    strncpy(
        _checksum,
        digest.c_str(),
        digest.size() + 1 );

    return 0;

} // chksumLocFile

int verifyChksumLocFile(
    char *fileName,
    const char *myChksum,
    char *chksumStr ) {
    // =-=-=-=-=-=-=-
    // extract scheme from checksum string
    std::string scheme;
    irods::error ret = irods::get_hash_scheme_from_checksum( myChksum, scheme );
    if ( !ret.ok() ) {
        //irods::log( PASS( ret ) );
    }

    char chksumBuf[CHKSUM_LEN];
    if ( chksumStr == NULL ) {
        chksumStr = chksumBuf;
    }

    int status = chksumLocFile( fileName, chksumStr, scheme.c_str() );
    if ( status < 0 ) {
        return status;
    }
    if ( strcmp( myChksum, chksumStr ) != 0 ) {
        return USER_CHKSUM_MISMATCH;
    }
    return 0;
}

int
hashToStr( unsigned char *digest, char *digestStr ) {
    int i;
    char *outPtr = digestStr;

    for ( i = 0; i < 16; i++ ) {
        sprintf( outPtr, "%02x", digest[i] );
        outPtr += 2;
    }

    return 0;
}

/* rcChksumLocFile - chksum a local file and put the result in the
 * condInput.
 * Input -
 *	char *fileName - the local file name
 *	char *chksumFlag - the chksum flag. valid flags are
 *         VERIFY_CHKSUM_KW and REG_CHKSUM_KW.
 *	keyValPair_t *condInput - the result is put into this struct
 */

int
rcChksumLocFile( char *fileName, char *chksumFlag, keyValPair_t *condInput, const char* _scheme ) {
    char chksumStr[NAME_LEN];
    int status;

    if ( condInput == NULL || chksumFlag == NULL || fileName == NULL ) {
        rodsLog( LOG_NOTICE,
                 "rcChksumLocFile: NULL input" );
        return USER__NULL_INPUT_ERR;
    }

    if ( strcmp( chksumFlag, VERIFY_CHKSUM_KW ) != 0 &&
            strcmp( chksumFlag, REG_CHKSUM_KW ) != 0 &&
            strcmp( chksumFlag, RSYNC_CHKSUM_KW ) != 0 ) {
        rodsLog( LOG_NOTICE,
                 "rcChksumLocFile: bad input chksumFlag %s", chksumFlag );
        return USER_BAD_KEYWORD_ERR;
    }

    status = chksumLocFile( fileName, chksumStr, _scheme );
    if ( status < 0 ) {
        return status;
    }

    addKeyVal( condInput, chksumFlag, chksumStr );

    return 0;
}
