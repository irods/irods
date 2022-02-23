#ifndef _SHA512_STRATEGY_HPP_
#define _SHA512_STRATEGY_HPP_

#include "irods/HashStrategy.hpp"

namespace irods {
    extern const std::string SHA512_NAME;
    class SHA512Strategy : public HashStrategy {
        public:
            SHA512Strategy() {};
            virtual ~SHA512Strategy() {};

            std::string name() const override {
                return SHA512_NAME;
            }
            error init( boost::any& context ) const override;
            error update( const std::string& data, boost::any& context ) const override;
            error digest( std::string& messageDigest, boost::any& context ) const override;
            bool isChecksum( const std::string& ) const override;

    };
} // namespace irods

#endif // _SHA512_STRATEGY_HPP_
