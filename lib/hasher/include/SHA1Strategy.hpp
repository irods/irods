#ifndef _SHA1_STRATEGY_HPP_
#define _SHA1_STRATEGY_HPP_

#include "HashStrategy.hpp"
#include <string>
#include <openssl/sha.h>

namespace irods {
    const std::string SHA1_NAME( "sha1" );
    class SHA1Strategy : public HashStrategy {
        public:
            SHA1Strategy() {};
            virtual ~SHA1Strategy() {};

            virtual std::string name() const {
                return SHA1_NAME;
            }
            virtual error init( boost::any& context ) const;
            virtual error update( const std::string& data, boost::any& context ) const;
            virtual error digest( std::string& messageDigest, boost::any& context ) const;
            virtual bool isChecksum( const std::string& ) const;

    };
}; // namespace irods

#endif // _SHA1_STRATEGY_HPP_
