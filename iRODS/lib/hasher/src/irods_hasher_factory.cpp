


#include "irods_hasher_factory.hpp"
#include "MD5Strategy.hpp"
#include "SHA256Strategy.hpp"
#include "md5Checksum.hpp"

namespace irods {

    error get_hash_scheme_from_checksum(
        const std::string& _chksum,
        std::string&       _scheme ) {
        if ( _chksum.empty() ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "empty chksum string" );
        }

        if ( std::string::npos != _chksum.find( SHA256_CHKSUM_PREFIX ) ) {
            _scheme = SHA256_NAME;
            return SUCCESS();
        }
        else if ( std::string::npos == _chksum.find_first_not_of( "0123456789abcdefABCDEF" ) ) {
            _scheme = MD5_NAME;
            return SUCCESS();
        }

        return ERROR(
                   SYS_INVALID_INPUT_PARAM,
                   "hash scheme not found" );

    } // get_hasher_scheme_from_checksum

    error hasher_factory( Hasher& _hasher ) {
        _hasher.addStrategy( new MD5Strategy() );

#ifdef SHA256_FILE_HASH
        _hasher.addStrategy( new SHA256Strategy() );
#endif
        return SUCCESS();

    } // hasher_factory

}; // namespace irods


