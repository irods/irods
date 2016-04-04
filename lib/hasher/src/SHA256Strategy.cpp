#include "SHA256Strategy.hpp"
#include "checksum.hpp"

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <boost/algorithm/string/predicate.hpp>
#include <openssl/sha.h>

#include "base64.h"

namespace irods {

    error
    SHA256Strategy::init( boost::any& _context ) const {
        _context = SHA256_CTX();
        SHA256_Init( boost::any_cast<SHA256_CTX>( &_context ) );
        return SUCCESS();
    }

    error
    SHA256Strategy::update( const std::string& data, boost::any& _context ) const {
        SHA256_Update( boost::any_cast<SHA256_CTX>( &_context ), data.c_str(), data.size() );
        return SUCCESS();
    }

    error
    SHA256Strategy::digest( std::string& _messageDigest, boost::any& _context ) const {
        unsigned char final_buffer[SHA256_DIGEST_LENGTH];
        SHA256_Final( final_buffer, boost::any_cast<SHA256_CTX>( &_context ) );
        int len = strlen( SHA256_CHKSUM_PREFIX );
        unsigned long out_len = CHKSUM_LEN - len;

        unsigned char out_buffer[CHKSUM_LEN];
        base64_encode( final_buffer, SHA256_DIGEST_LENGTH, out_buffer, &out_len );

        _messageDigest = SHA256_CHKSUM_PREFIX;
        _messageDigest += std::string( ( char* )out_buffer, out_len );

        return SUCCESS();
    }

    bool
    SHA256Strategy::isChecksum( const std::string& _chksum ) const {
        return boost::starts_with( _chksum, SHA256_CHKSUM_PREFIX );
    }
}; // namespace irods
