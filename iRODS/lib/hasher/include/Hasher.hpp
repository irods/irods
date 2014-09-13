#ifndef _Hasher_H_
#define _Hasher_H_

#include "HashStrategy.hpp"
#include "irods_error.hpp"

#include <string>
#include <boost/any.hpp>

namespace irods {

    const std::string STRICT_HASH_POLICY( "strict" );
    const std::string COMPATIBLE_HASH_POLICY( "compatible" );

    class Hasher {
    public:
        Hasher() : _strategy( NULL ) {}

        error init( const HashStrategy* );
        error update( const std::string& );
        error digest( std::string& messageDigest );

    private:
        const HashStrategy* _strategy;
        boost::any          _context;
        error               _stored_error;
        std::string         _stored_digest;
    };

}; // namespace irods

#endif // _Hasher_H_
