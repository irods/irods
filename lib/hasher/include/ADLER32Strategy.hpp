#ifndef _ADLER32_STRATEGY_HPP_
#define _ADLER32_STRATEGY_HPP_

#include "HashStrategy.hpp"
#include <string>
#include <openssl/sha.h>

namespace irods {
    const std::string ADLER32_NAME( "adler32" );
    class ADLER32Strategy : public HashStrategy {
        public:
            ADLER32Strategy() {};
            virtual ~ADLER32Strategy() {};

            virtual std::string name() const {
                return ADLER32_NAME;
            }
            virtual error init( boost::any& context ) const;
            virtual error update( const std::string& data, boost::any& context ) const;
            virtual error digest( std::string& messageDigest, boost::any& context ) const;
            virtual bool isChecksum( const std::string& ) const;

    };
}; // namespace irods

#endif // _ADLER32_STRATEGY_HPP_
