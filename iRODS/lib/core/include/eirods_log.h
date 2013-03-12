/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */




#ifndef __EIRODS_LOG_H__
#define __EIRODS_LOG_H__

// =-=-=-=-=-=-=-
// irods includes
#include "rodsLog.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_error.h"

#include <string>
#include <iostream>

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

#define DEBUGMSG(msg)                                           \
    {                                                           \
    std::stringstream ss;                                       \
    ss << msg << " " << __FUNCTION__ << " " << __FILE__ << ":" << __LINE__; \
    eirods::log(LOG_NOTICE, ss.str());                                  \
    }
    
#endif // __EIRODS_LOG_H__



