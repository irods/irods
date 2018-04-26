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
            ~SHA256Strategy() override {};

            std::string name() const override {
                return SHA256_NAME;
            }
            error init( boost::any& context ) const override;
            error update( const std::string& data, boost::any& context ) const override;
            error digest( std::string& messageDigest, boost::any& context ) const override;
            bool isChecksum( const std::string& ) const override;

    };
}; // namespace irods

#endif // _SHA256_STRATEGY_HPP_
