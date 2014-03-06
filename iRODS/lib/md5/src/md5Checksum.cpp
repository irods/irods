/* md5Checksum.c - checksumming routine on the client side
 */

#include "md5Checksum.hpp"
#include "rcMisc.hpp"
#include "irods_stacktrace.hpp"
#include "irods_hasher_factory.hpp"
#include "MD5Strategy.hpp"
#include "getRodsEnv.hpp"

#define MD5_BUF_SZ      (4 * 1024)

#ifdef MD5_TESTING

int main( int argc, char *argv[] ) {
    int i = 1; // JMC cppcheck - uninit var
    char chksumStr[NAME_LEN];

    if ( argc != 2 ) {
        fprintf( stderr, "usage: md5checksum localFile\n" );
        exit( 1 );
    }

    status = chksumLocFile( argv[i], chksumStr );

    if ( status >= 0 ) {
        printf( "chksumStr = %s\n", chksumStr );
        exit( 0 );
    }
    else {
        fprintf( stderr, "chksumFile error for %s, status = %d.\n",
                 argv[i], status );
        exit( 1 );
    }
}

#endif 	/* MD5_TESTING */

int
chksumLocFile( char *fileName, char *chksumStr, const char* scheme ) {
    FILE *file = 0;
    int len = 0;
    char buffer[MD5_BUF_SZ];
    // =-=-=-=-=-=-=-
    // capture client side configuration
    rodsEnv env;
    int status = getRodsEnv( &env );
    if ( status < 0 ) {
        return status;
    }

    // =-=-=-=-=-=-=-
    // capture the configured scheme if it is valid
    std::string env_scheme( irods::MD5_NAME );
    if ( strlen( env.rodsDefaultHashScheme ) > 0 ) {
        env_scheme = env.rodsDefaultHashScheme;
    }

    // =-=-=-=-=-=-=-
    // capture the configured hash match policy if it is valid
    std::string env_policy;
    if ( strlen( env.rodsMatchHashPolicy ) > 0 ) {
        env_policy = env.rodsMatchHashPolicy;
    }

    // =-=-=-=-=-=-=-
    // capture the incoming scheme if it is valid
    std::string hash_scheme;
    if ( scheme &&
            strlen( scheme ) > 0 &&
            strlen( scheme ) < NAME_LEN ) {
        hash_scheme = scheme;
    }

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
    // open the local file
    if ( ( file = fopen( fileName, "rb" ) ) == NULL ) {
        status = UNIX_FILE_OPEN_ERR - errno;
        rodsLogError( LOG_NOTICE, status,
                      "chksumFile; fopen failed for %s. status = %d", fileName, status );
        return ( status );
    }

    // =-=-=-=-=-=-=-
    // init the hasher object
    irods::Hasher hasher;
    irods::error ret = irods::hasher_factory( hasher );

    hasher.init( final_scheme );
    while ( ( len = fread( buffer, 1, MD5_BUF_SZ, file ) ) > 0 ) {
        hasher.update( buffer, len );
    }
    fclose( file );

    // =-=-=-=-=-=-=-
    // capture the digest
    std::string digest;
    hasher.digest( digest );
    strncpy( chksumStr, digest.c_str(), digest.size() + 1 );
    return ( 0 );
}

int verifyChksumLocFile(
    char *fileName,
    char *myChksum,
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
        return ( status );
    }
    if ( strcmp( myChksum, chksumStr ) != 0 ) {
        return ( USER_CHKSUM_MISMATCH );
    }
    return 0;
}


int
md5ToStr( unsigned char *digest, char *chksumStr ) {
    int i;
    char *outPtr = chksumStr;

    for ( i = 0; i < 16; i++ ) {
        sprintf( outPtr, "%02x", digest[i] );
        outPtr += 2;
    }

    return ( 0 );
}

int
hashToStr( unsigned char *digest, char *digestStr ) {
    int i;
    char *outPtr = digestStr;

    for ( i = 0; i < 16; i++ ) {
        sprintf( outPtr, "%02x", digest[i] );
        outPtr += 2;
    }

    return ( 0 );
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
        return ( USER__NULL_INPUT_ERR );
    }

    if ( strcmp( chksumFlag, VERIFY_CHKSUM_KW ) != 0 &&
            strcmp( chksumFlag, REG_CHKSUM_KW ) != 0 &&
            strcmp( chksumFlag, RSYNC_CHKSUM_KW ) != 0 ) {
        rodsLog( LOG_NOTICE,
                 "rcChksumLocFile: bad input chksumFlag %s", chksumFlag );
        return ( USER_BAD_KEYWORD_ERR );
    }

    status = chksumLocFile( fileName, chksumStr, _scheme );

    if ( status < 0 ) {
        return ( status );
    }

    addKeyVal( condInput, chksumFlag, chksumStr );

    return ( 0 );
}

