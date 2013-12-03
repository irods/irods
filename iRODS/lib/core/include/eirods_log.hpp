/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */



#ifndef __EIRODS_LOG_H__
#define __EIRODS_LOG_H__

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_error.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <string>

namespace eirods {
class error;
    // =-=-=-=-=-=-=-
    // provide our own interface which will take our error which can
    // be refactored later
    void log( error );

    // =-=-=-=-=-=-=-
    // provide our own interface which will take stl types which can
    // be refactored later
    void log( int, std::string );

}; // namespace eirods

#define DEBUGMSG(msg)                                           \
    {                                                           \
    std::stringstream ss;                                       \
    ss << msg << " " << __FUNCTION__ << " " << __FILE__ << ":" << __LINE__; \
    eirods::log(LOG_NOTICE, ss.str());                                  \
    }
    
#endif // __EIRODS_LOG_H__



