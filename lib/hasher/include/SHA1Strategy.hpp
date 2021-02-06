#ifndef _SHA1_STRATEGY_HPP_
#define _SHA1_STRATEGY_HPP_

#include "HashStrategy.hpp"

namespace irods {
    extern const std::string SHA1_NAME;
    class SHA1Strategy : public HashStrategy {
        public:
            SHA1Strategy() {};
            virtual ~SHA1Strategy() {};

            std::string name() const override {
                return SHA1_NAME;
            }
            error init( boost::any& context ) const override;
            error update( const std::string& data, boost::any& context ) const override;
            error digest( std::string& messageDigest, boost::any& context ) const override;
            bool isChecksum( const std::string& ) const override;

    };
} // namespace irods

#endif // _SHA1_STRATEGY_HPP_
