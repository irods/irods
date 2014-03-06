/* -*- mode: c++; fill-column: 72; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "SHA256Strategy.hpp"
#include "md5Checksum.hpp"

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string.h>

#include "base64.hpp"

namespace irods {

    std::string SHA256Strategy::_name = SHA256_NAME;

    SHA256Strategy::
    SHA256Strategy( void ) {
        _finalized = false;
    }

    SHA256Strategy::
    ~SHA256Strategy( void ) {
        // TODO - stub
    }

    unsigned int SHA256Strategy::
    init( void ) {
        unsigned int result = 0;
        SHA256_Init( &_context );
        _finalized = false;
        return result;
    }

    unsigned int SHA256Strategy::
    update(
        char const* data,
        unsigned int size ) {
        unsigned int result = 0;
        if ( !_finalized ) {
            unsigned char* charData = new unsigned char[size];
            memcpy( charData, data, size );
            SHA256_Update( &_context, charData, size );
            delete [] charData;
        }
        else {
            result = 1;             // TODO - should be an enum or string
            // table value here
        }
        return result;
    }

    unsigned int SHA256Strategy::
    digest(
        std::string& messageDigest ) {
        unsigned int result = 0;
        if ( !_finalized ) {
            // =-=-=-=-=-=-=-
            // capture the final buffer
            unsigned char final_buffer[SHA256_DIGEST_LENGTH];
            SHA256_Final( final_buffer, &_context );

            // =-=-=-=-=-=-=-
            // compute output length
            int len = strlen( SHA256_CHKSUM_PREFIX );
            unsigned long out_len = CHKSUM_LEN - len;

            // =-=-=-=-=-=-=-
            // base64 encode the digest
            unsigned char out_buffer[CHKSUM_LEN];
            base64_encode( final_buffer, SHA256_DIGEST_LENGTH, out_buffer, &out_len );

            // =-=-=-=-=-=-=-
            // iterator based ctor to convert unsigned char[] to string
            std::string tmp_buff( out_buffer, out_buffer + out_len );

            // =-=-=-=-=-=-=-
            // build final tagged output digest
            _digest += SHA256_CHKSUM_PREFIX;
            _digest += tmp_buff;

        }
        messageDigest = _digest;
        return result;
    }
}; // namespace irods
