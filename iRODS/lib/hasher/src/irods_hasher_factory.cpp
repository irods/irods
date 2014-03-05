


#include "irods_hasher_factory.hpp"
#include "MD5Strategy.hpp"
#include "SHA256Strategy.hpp"
#include "md5Checksum.hpp"

namespace irods {

    error get_hash_scheme_from_checksum(
        const std::string& _chksum,
        std::string&       _scheme ) {
        if ( _chksum.empty() ) {

        }

        if ( std::string::npos != _chksum.find( SHA256_CHKSUM_PREFIX ) ) {
            _scheme = SHA256_NAME;
        }
        else {
            _scheme = MD5_NAME;
        }

        return SUCCESS();

    } // get_hasher_scheme_from_checksum

    error hasher_factory( Hasher& _hasher ) {
        _hasher.addStrategy( new MD5Strategy() );

#ifdef SHA256_FILE_HASH
        _hasher.addStrategy( new SHA256Strategy() );
#endif
        return SUCCESS();

    } // hasher_factory

}; // namespace irods


