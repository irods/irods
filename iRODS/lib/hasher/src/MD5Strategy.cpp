#include "MD5Strategy.hpp"

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <string.h>
#include <openssl/md5.h>
#include "irods_stacktrace.hpp"

namespace irods {

    error
    MD5Strategy::init( boost::any& _context ) const {
        _context = MD5_CTX();
        MD5_Init( boost::any_cast<MD5_CTX>( &_context ) );
        return SUCCESS();
    }

    error
    MD5Strategy::update( const std::string& data, boost::any& _context ) const {
        MD5_Update( boost::any_cast<MD5_CTX>( &_context ), ( const unsigned char * )data.c_str(), data.size() );
        return SUCCESS();
    }

    error
    MD5Strategy::digest( std::string& messageDigest, boost::any& _context ) const {
        unsigned char buffer[17];
        MD5_Final( buffer, boost::any_cast<MD5_CTX>( &_context ) );
        std::stringstream ins;
        for ( int i = 0; i < 16; ++i ) {
            ins << std::setfill( '0' ) << std::setw( 2 ) << std::hex << ( int )buffer[i];
        }
        messageDigest = ins.str();
        return SUCCESS();
    }

    bool
    MD5Strategy::isChecksum( const std::string& _chksum ) const {
        return std::string::npos == _chksum.find_first_not_of( "0123456789abcdefABCDEF" );
    }
}; //namespace irods
