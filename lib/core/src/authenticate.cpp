#include "irods/authenticate.h"

#include "irods/authentication_plugin_framework.hpp"
#include "irods/getRodsEnv.h"
#include "irods/rcConnect.h"
#include "irods/rodsError.h"

#include <nlohmann/json.hpp>

auto rc_authenticate_client(RcComm* _comm, const char* _context) -> int
{
    namespace ia = irods::authentication;

    // No nullptrs allowed.
    if (nullptr == _comm || nullptr == _context) {
        return INVALID_INPUT_ARGUMENT_NULL_POINTER;
    }

    // If _comm already claims to be authenticated, there is nothing to do.
    if (1 == _comm->loggedIn) {
        return 0;
    }

    try {
        return irods::authentication::authenticate_client(*_comm, nlohmann::json::parse(_context));
    }
    catch (const irods::exception& e) {
        return static_cast<int>(e.code());
    }
    catch (const nlohmann::json::exception& e) {
        return JSON_VALIDATION_ERROR;
    }
    catch (const std::exception& e) {
        return SYS_LIBRARY_ERROR;
    }
} // rc_authenticate_client
