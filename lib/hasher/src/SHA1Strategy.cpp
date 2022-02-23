#include "irods/SHA1Strategy.hpp"
#include "irods/checksum.h"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <boost/algorithm/string/predicate.hpp>
#include <openssl/sha.h>

#include "irods/base64.h"

namespace irods {

    const std::string SHA1_NAME( "sha1" );

    error
    SHA1Strategy::init( boost::any& _context ) const {
        _context = SHA_CTX();
        SHA1_Init( boost::any_cast<SHA_CTX>( &_context ) );
        return SUCCESS();
    }

    error
    SHA1Strategy::update( const std::string& data, boost::any& _context ) const {
        SHA1_Update( boost::any_cast<SHA_CTX>( &_context ), data.c_str(), data.size() );
        return SUCCESS();
    }

    error
    SHA1Strategy::digest( std::string& _messageDigest, boost::any& _context ) const {
        unsigned char final_buffer[SHA_DIGEST_LENGTH];
        SHA1_Final( final_buffer, boost::any_cast<SHA_CTX>( &_context ) );
        int len = strlen( SHA1_CHKSUM_PREFIX );
        unsigned long out_len = CHKSUM_LEN - len;

        unsigned char out_buffer[CHKSUM_LEN];
        base64_encode( final_buffer, SHA_DIGEST_LENGTH, out_buffer, &out_len );

        _messageDigest = SHA1_CHKSUM_PREFIX;
        _messageDigest += std::string( ( char* )out_buffer, out_len );

        return SUCCESS();
    }

    bool
    SHA1Strategy::isChecksum( const std::string& _chksum ) const {
        return boost::starts_with( _chksum, SHA1_CHKSUM_PREFIX );
    }
}; // namespace irods
