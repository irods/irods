


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
    void log( int, std::string );

    // =-=-=-=-=-=-=-
	// provide our own interface which will take our error which can
	// be refactored later
	void log( eirods::error );

}; // namespace eirods


#endif // __EIRODS_LOG_H__



