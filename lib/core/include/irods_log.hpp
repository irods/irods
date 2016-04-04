#ifndef __IRODS_LOG_HPP__
#define __IRODS_LOG_HPP__

// =-=-=-=-=-=-=-
#include "irods_error.hpp"
#include "rodsLog.h"
#include "stringOpr.h"

// =-=-=-=-=-=-=-
// stl includes
#include <string>

#include <time.h>
#include <sys/time.h>

namespace irods {
    class error;
// =-=-=-=-=-=-=-
// provide our own interface which will take our error which can
// be refactored later
    void log( error );

// =-=-=-=-=-=-=-
// provide our own interface which will take stl types which can
// be refactored later
    void log( int, std::string );

}; // namespace irods

#define DEBUGMSG(msg)                                           \
    {                                                           \
    time_t timeValue;                                           \
    char timeBuf[100]; \
    time( &timeValue ); \
    rstrcpy( timeBuf, ctime( &timeValue ), 90 ); \
    timeBuf[19] = '\0'; \
    std::stringstream ss;                                                   \
    ss << timeBuf << " " << msg << " " << __FUNCTION__ << " " << __FILE__ << ":" << __LINE__; \
    irods::log(LOG_NOTICE, ss.str());                                   \
    std::cerr << ss.str() << std::endl; \
    }

#endif // __IRODS_LOG_HPP__



