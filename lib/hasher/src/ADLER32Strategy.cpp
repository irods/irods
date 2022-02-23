#include "irods/ADLER32Strategy.hpp"
#include "irods/checksum.h"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <boost/algorithm/string/predicate.hpp>

#include "irods/base64.h"

namespace irods {

    const std::string ADLER32_NAME( "adler32" );

    struct adler32_parts {
        uint32_t a;
        uint32_t b;
    };

    adler32_parts adler32_init() {
        return adler32_parts{1, 0};
    }

    static adler32_parts adler32_update(const adler32_parts& parts, const unsigned char *data, size_t len) {

        const uint32_t MOD_ADLER = 65521;

        uint32_t a = parts.a, b = parts.b;

        // Process each byte of the data in order
        for (size_t index = 0; index < len; ++index)
        {
            a = (a + data[index]) % MOD_ADLER;
            b = (b + a) % MOD_ADLER;
        }

        return adler32_parts{a, b};
    }

    static uint32_t adler32_final(const adler32_parts& parts) {
        return (parts.b << 16) | parts.a;
    }


    error
    ADLER32Strategy::init( boost::any& _context ) const {
        _context = adler32_init();
        return SUCCESS();
    }

    error
    ADLER32Strategy::update( const std::string& data, boost::any& _context ) const {

        _context = adler32_update(boost::any_cast<adler32_parts>(_context), reinterpret_cast<const unsigned char*>(data.c_str()), data.size());
        return SUCCESS();
    }

    error
    ADLER32Strategy::digest( std::string& _messageDigest, boost::any& _context ) const {

        const unsigned int ADLER32_DIGEST_LENGTH = 4;

        uint32_t result = adler32_final(boost::any_cast<adler32_parts>(_context));

        std::stringstream ss;
        ss << std::setfill('0') << std::hex << std::setw(ADLER32_DIGEST_LENGTH * 2) << result;

        _messageDigest = ADLER32_CHKSUM_PREFIX;
        _messageDigest += ss.str();

        return SUCCESS();
    }

    bool
    ADLER32Strategy::isChecksum( const std::string& _chksum ) const {
        return boost::starts_with( _chksum, ADLER32_CHKSUM_PREFIX );
    }
}; // namespace irods
