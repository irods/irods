#include "SHA512Strategy.hpp"
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
    SHA512Strategy::init( boost::any& _context ) const {
        _context = SHA512_CTX();
        SHA512_Init( boost::any_cast<SHA512_CTX>( &_context ) );
        return SUCCESS();
    }

    error
    SHA512Strategy::update( const std::string& data, boost::any& _context ) const {
        SHA512_Update( boost::any_cast<SHA512_CTX>( &_context ), data.c_str(), data.size() );
        return SUCCESS();
    }

    error
    SHA512Strategy::digest( std::string& _messageDigest, boost::any& _context ) const {
        unsigned char final_buffer[SHA512_DIGEST_LENGTH];
        SHA512_Final( final_buffer, boost::any_cast<SHA512_CTX>( &_context ) );
        int len = strlen( SHA512_CHKSUM_PREFIX );
        unsigned long out_len = CHKSUM_LEN * 2 - len;

        unsigned char out_buffer[CHKSUM_LEN * 2];
        base64_encode( final_buffer, SHA512_DIGEST_LENGTH, out_buffer, &out_len );

        _messageDigest = SHA512_CHKSUM_PREFIX;
        _messageDigest += std::string( ( char* )out_buffer, out_len );

        return SUCCESS();
    }

    bool
    SHA512Strategy::isChecksum( const std::string& _chksum ) const {
        return boost::starts_with( _chksum, SHA512_CHKSUM_PREFIX );
    }
}; // namespace irods
