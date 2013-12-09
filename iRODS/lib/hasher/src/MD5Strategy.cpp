/* -*- mode: c++; fill-column: 72; c-basic-offset: 4; indent-tabs-mode: nil -*- */


#include "MD5Strategy.hpp"

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <string.h>

namespace irods {

    std::string MD5Strategy::_name = "MD5";

    MD5Strategy::
    MD5Strategy( void ) {
        _finalized = false;
    }

    MD5Strategy::
    ~MD5Strategy() {
        // TODO - stub
    }

    unsigned int MD5Strategy::
    init( void ) {
        unsigned int result = 0;
        MD5Init( &_context );
        _finalized = false;
        return result;
    }

    unsigned int MD5Strategy::
    update(
        char const* data,
        unsigned int size ) {
        unsigned int result = 0;
        if ( !_finalized ) {
            unsigned char* charData = new unsigned char[size]; // overkill?
            memcpy( charData, data, size );
            MD5Update( &_context, charData, size );
            delete [] charData;
        }
        else {
            result = 1;             // TODO - should be an enum or string
            // table value here
        }
        return result;
    }

    unsigned int MD5Strategy::
    digest(
        std::string& messageDigest ) {
        unsigned int result = 0;
        if ( !_finalized ) {
            unsigned char buffer[17];
            MD5Final( buffer, &_context );
            std::stringstream ins;
            for ( int i = 0; i < 16; ++i ) {
                ins << std::setfill( '0' ) << std::setw( 2 ) << std::hex << ( int )buffer[i];
            }
            _digest = ins.str();
        }
        messageDigest = _digest;
        return result;
    }
}; //namespace irods
