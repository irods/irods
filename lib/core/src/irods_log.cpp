#include "irods_log.hpp"
#include "rodsLog.h"

namespace irods {
    void log(const int _level, const std::string& _msg) {
        rodsLog(_level, _msg.c_str());
    }

    void log(const irods::error& _err) {
        if (_err.ok()) {
            log( LOG_NOTICE, _err.result() );
        }
        else {
            log( LOG_ERROR, _err.result() );
        }
    }
}
