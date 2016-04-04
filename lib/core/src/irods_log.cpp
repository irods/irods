// =-=-=-=-=-=-=-
#include "irods_log.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "rodsLog.h"

namespace irods {

// =-=-=-=-=-=-=-
// provide our own interface which will take stl types which can
// be refactored later
    void log( int _level, std::string _msg ) {
        rodsLog( _level, const_cast< char* >( _msg.c_str() ) );
    } // log

// =-=-=-=-=-=-=-
// provide our own interface which will take our error which can
// be refactored later
    void log( irods::error _err ) {
        if ( _err.ok() ) {
            log( LOG_NOTICE, _err.result() );
        }
        else {
            log( LOG_ERROR, _err.result() );
        }
    } // log

}; // namespace irods



