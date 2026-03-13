#ifndef _HASHER_HPP_
#define _HASHER_HPP_

/// \file

#include "irods/HashStrategy.hpp"
#include "irods/irods_error.hpp"

#include <string>
#include <boost/any.hpp>

namespace irods {

    namespace hash
    {
        struct options;
    } // namespace hash

    const std::string STRICT_HASH_POLICY( "strict" );
    const std::string COMPATIBLE_HASH_POLICY( "compatible" );

    class Hasher {
        public:
            Hasher() : _strategy( NULL ) {}
            ~Hasher()
            {
                if (nullptr != _strategy) {
                    _strategy->free_context(_context);
                }
            }
            error init( const HashStrategy* );
            error update( const std::string& );
            error digest( std::string& messageDigest );

            /// Produce a digest in string form based on the provided options.
            ///
            /// \param[in] _options \p irods::hash::options specifying how the output string should be formatted.
            /// \param[out] _out The output string.
            ///
            /// \return irods::error Any errors that are encountered are held in the returned \p irods::error.
            ///
            /// \since 5.1.0
            auto digest(const hash::options& _options, std::string& _out) -> error;

          private:
            const HashStrategy* _strategy;
            boost::any          _context;
            error               _stored_error;
            std::string         _stored_digest;
    };

}; // namespace irods

#endif // _HASHER_HPP_
