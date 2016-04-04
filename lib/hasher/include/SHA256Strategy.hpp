#ifndef _SHA256_STRATEGY_HPP_
#define _SHA256_STRATEGY_HPP_

#include "HashStrategy.hpp"
#include <string>
#include <openssl/sha.h>

namespace irods {
    const std::string SHA256_NAME( "sha256" );
    class SHA256Strategy : public HashStrategy {
        public:
            SHA256Strategy() {};
            virtual ~SHA256Strategy() {};

            virtual std::string name() const {
                return SHA256_NAME;
            }
            virtual error init( boost::any& context ) const;
            virtual error update( const std::string& data, boost::any& context ) const;
            virtual error digest( std::string& messageDigest, boost::any& context ) const;
            virtual bool isChecksum( const std::string& ) const;

    };
}; // namespace irods

#endif // _SHA256_STRATEGY_HPP_
