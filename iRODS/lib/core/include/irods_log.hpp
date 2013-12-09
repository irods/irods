/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */



#ifndef __IRODS_LOG_HPP__
#define __IRODS_LOG_HPP__

// =-=-=-=-=-=-=-
#include "irods_error.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <string>

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
    std::stringstream ss;                                       \
    ss << msg << " " << __FUNCTION__ << " " << __FILE__ << ":" << __LINE__; \
    irods::log(LOG_NOTICE, ss.str());                                  \
    }

#endif // __IRODS_LOG_HPP__



