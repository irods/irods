#ifndef _HASH_STRATEGY_HPP_
#define _HASH_STRATEGY_HPP_

#include <irods_error.hpp>
#include <string>
#include <boost/any.hpp>

namespace irods {

    class HashStrategy {
        public:

            virtual ~HashStrategy() {};

            virtual std::string name() const = 0;
            virtual error init( boost::any& context ) const = 0;
            virtual error update( const std::string&, boost::any& context ) const = 0;
            virtual error digest( std::string& messageDigest, boost::any& context ) const = 0;
            virtual bool isChecksum( const std::string& ) const = 0;
    };
}; // namespace irods

#endif // _HASH_STRATEGY_HPP_
