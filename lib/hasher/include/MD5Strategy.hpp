#ifndef _MD5_STRATEGY_HPP_
#define _MD5_STRATEGY_HPP_

#include "HashStrategy.hpp"

namespace irods {
    extern const std::string MD5_NAME;
    class MD5Strategy : public HashStrategy {
        public:
            MD5Strategy() {};
            virtual ~MD5Strategy() {};

            std::string name() const override {
                return MD5_NAME;
            }
            error init( boost::any& context ) const override;
            error update( const std::string&, boost::any& context ) const override;
            error digest( std::string& messageDigest, boost::any& context ) const override;
            bool isChecksum( const std::string& ) const override;

    };
} // namespace irods

#endif // _MD5_STRATEGY_HPP_
