#include "irods_tmp_string.hpp"

#include <string.h>

namespace irods {

    tmp_string::tmp_string(
        const char* orig ) : string_( 0 ) {

        if ( orig != 0 ) {
            int length = strlen( orig );
            string_ = new char[length + 1];
            strncpy( string_, orig, length + 1 );
        }
    }


    tmp_string::~tmp_string( void ) {
        if ( string_ != 0 ) {
            delete [] string_;
        }
    }

}; // namespace irods
