#ifndef _SHA512_STRATEGY_HPP_
#define _SHA512_STRATEGY_HPP_

#include "HashStrategy.hpp"
#include <string>
#include <openssl/sha.h>

namespace irods {
    const std::string SHA512_NAME( "sha512" );
    class SHA512Strategy : public HashStrategy {
        public:
            SHA512Strategy() {};
            virtual ~SHA512Strategy() {};

            virtual std::string name() const {
                return SHA512_NAME;
            }
            virtual error init( boost::any& context ) const;
            virtual error update( const std::string& data, boost::any& context ) const;
            virtual error digest( std::string& messageDigest, boost::any& context ) const;
            virtual bool isChecksum( const std::string& ) const;

    };
}; // namespace irods

#endif // _SHA512_STRATEGY_HPP_
