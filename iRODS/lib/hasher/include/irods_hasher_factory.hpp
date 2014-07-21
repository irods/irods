#ifndef __IRODS_HASHER_FACTORY_HPP__
#define __IRODS_HASHER_FACTORY_HPP__

#include "irods_error.hpp"
#include "Hasher.hpp"

namespace irods {

const std::string STRICT_HASH_POLICY( "strict" );
const std::string COMPATIBLE_HASH_POLICY( "compatible" );

error get_hash_scheme_from_checksum(
    const std::string&, // checksum
    std::string& );     // scheme
error hasher_factory( Hasher& );

}; // namespace irods

#endif // __IRODS_HASHER_FACTORY_HPP__

