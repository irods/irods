#include "irods_hasher_factory.hpp"
#include "checksum.hpp"
#include "MD5Strategy.hpp"
#include "SHA256Strategy.hpp"
#include "SHA512Strategy.hpp"
#include "ADLER32Strategy.hpp"
#include "SHA1Strategy.hpp"
#include "rodsErrorTable.h"
#include <sstream>
#include <boost/unordered_map.hpp>

namespace irods {

    namespace {
        const SHA256Strategy _sha256;
        const SHA512Strategy _sha512;
        const ADLER32Strategy _adler32;
        const MD5Strategy _md5;
        const SHA1Strategy _sha1;

        boost::unordered_map<const std::string, const HashStrategy*>
        make_map() {
            boost::unordered_map<const std::string, const HashStrategy*> map;
            map[ SHA256_NAME ] = &_sha256;
            map[ SHA512_NAME ] = &_sha512;
            map[ MD5_NAME ] = &_md5;
            map[ ADLER32_NAME ] = &_adler32;
            map[ SHA1_NAME ] = &_sha1;
            return map;
        }

        const boost::unordered_map<const std::string, const HashStrategy*> _strategies( make_map() );
    };

    error
    getHasher( const std::string& _name, Hasher& _hasher ) {
        boost::unordered_map<const std::string, const HashStrategy*>::const_iterator it = _strategies.find( _name );
        if ( _strategies.end() == it ) {
            std::stringstream msg;
            msg << "Unknown hashing scheme [" << _name << "]";
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
        }
        _hasher.init( it->second );
        return SUCCESS();
    }

    error
    get_hash_scheme_from_checksum(
        const std::string& _chksum,
        std::string&       _scheme ) {
        if ( _chksum.empty() ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "empty chksum string" );
        }
        for ( boost::unordered_map<const std::string, const HashStrategy*>::const_iterator it = _strategies.begin();
                _strategies.end() != it; ++it ) {
            if ( it->second->isChecksum( _chksum ) ) {
                _scheme = it->second->name();
                return SUCCESS();
            }
        }
        return ERROR(
                   SYS_INVALID_INPUT_PARAM,
                   "hash scheme not found" );

    } // get_hasher_scheme_from_checksum

}; // namespace irods


