#include "irods/irods_log.hpp"
#include "irods/rcGlobalExtern.h"
#include "irods/rodsLog.h"

namespace irods {
    void log(const int _level, const std::string& _msg) {
        rodsLog(_level, "%s", _msg.c_str());
    }

    void log(const int _level, const char *s) {
        rodsLog(_level, "%s", s);
    }

    void log(const irods::error& _err) {
        if (_err.ok()) {
            log(LOG_NOTICE, _err.result());
        } else {
            log(LOG_ERROR, _err.result());
        }
    }

    void log(const irods::exception& e) {
        if (CLIENT_PT == ::ProcessType) {
            log(LOG_ERROR, e.client_display_what());
        }
        else {
            log(LOG_ERROR, e.what());
        }
    }
}
