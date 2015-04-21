#ifndef _MD5_STRATEGY_HPP_
#define _MD5_STRATEGY_HPP_

#include "HashStrategy.hpp"

namespace irods {
    const std::string MD5_NAME( "md5" );
    class MD5Strategy : public HashStrategy {
        public:
            MD5Strategy() {};
            virtual ~MD5Strategy() {};

            virtual std::string name() const {
                return MD5_NAME;
            }
            virtual error init( boost::any& context ) const;
            virtual error update( const std::string&, boost::any& context ) const;
            virtual error digest( std::string& messageDigest, boost::any& context ) const;
            virtual bool isChecksum( const std::string& ) const;

    };
}; // namespace irods

#endif // _MD5_STRATEGY_HPP_
