


#ifndef __EIRODS_LOG_H__
#define __EIRODS_LOG_H__

// =-=-=-=-=-=-=-
// irods includes
#include "rodsLog.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_error.h"

namespace eirods {

    // =-=-=-=-=-=-=-
	// provide our own interface which will take stl types which can
	// be refactored later
    static void log( int _level, std::string _msg ) {
        rodsLog( _level, const_cast< char* >( _msg.c_str() ) );
	} // log

    // =-=-=-=-=-=-=-
	// provide our own interface which will take our error which can
	// be refactored later
	static void log( eirods::error _err ) {
        if( _err.ok() ) {
            log( LOG_NOTICE, _err.result() );
		} else {
            log( LOG_ERROR, _err.result() );
		}
	} // log

}; // namespace eirods


#endif // __EIRODS_LOG_H__



