#ifndef __IRODS_HASHER_FACTORY_HPP__
#define __IRODS_HASHER_FACTORY_HPP__

#include "irods_error.hpp"
#include "Hasher.hpp"

namespace irods {

    error getHasher( const std::string& name, Hasher& hasher );
    error get_hash_scheme_from_checksum(
        const std::string& checksum,
        std::string& scheme );

}; // namespace irods

#endif // __IRODS_HASHER_FACTORY_HPP__
