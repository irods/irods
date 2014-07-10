#ifndef _SHA256Strategy_H_
#define _SHA256Strategy_H_

#include "HashStrategy.hpp"

#include <string>

#include <openssl/sha.h>

namespace irods {
    const std::string SHA256_NAME( "sha256" );
    class SHA256Strategy : public HashStrategy {
    public:
        SHA256Strategy( void );
        virtual ~SHA256Strategy( void );

        virtual std::string name( void ) const {
            return _name;
        }
        virtual unsigned int init( void );
        virtual unsigned int update( char const* data, unsigned int size );
        virtual unsigned int digest( std::string& messageDigest );

    private:
        static std::string _name;

        SHA256_CTX _context;
        bool _finalized;
        std::string _digest;
    };
}; // namespace irods

#endif // _SHA256Strategy_H_
