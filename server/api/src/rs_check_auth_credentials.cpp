#include "irods/rs_check_auth_credentials.hpp"

#include "irods/catalog_utilities.hpp"
#include "irods/check_auth_credentials.h"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/irods_logger.hpp"
#include "irods/rodsErrorTable.h"
#include "irods/stringOpr.h" // For is_empty_string.

#include <cstdlib> // For std::malloc.

namespace
{
    using log_api = irods::experimental::log::api;
} // anonymous namespace

auto rs_check_auth_credentials(RsComm* _comm, CheckAuthCredentialsInput* _input, int** _correct) -> int
{
    if (!_comm || !_input || !_correct) {
        log_api::error("{}: Invalid input. Received one or more null pointers.", __func__);
        return SYS_INVALID_INPUT_PARAM;
    }

    *_correct = nullptr;

    // clang-format off
    if ((is_empty_string(_input->username, sizeof(CheckAuthCredentialsInput::username)) == 1) ||
        (is_empty_string(_input->zone, sizeof(CheckAuthCredentialsInput::zone)) == 1) ||
        (is_empty_string(_input->password, sizeof(CheckAuthCredentialsInput::password)) == 1))
    {
        log_api::error("{}: Invalid input. Received one or more empty strings.", __func__);
        return SYS_INVALID_INPUT_PARAM;
    }
    // clang-format on

    // Redirect to the provider.
    try {
        namespace ic = irods::experimental::catalog;

        if (!ic::connected_to_catalog_provider(*_comm)) {
            log_api::trace("{}: Redirecting request to catalog service provider.", __func__);
            auto* host_info = ic::redirect_to_catalog_provider(*_comm);
            return rc_check_auth_credentials(host_info->conn, _input, _correct);
        }

        ic::throw_if_catalog_provider_service_role_is_invalid();
    }
    catch (const irods::exception& e) {
        log_api::error(e.what());
        // NOLINTNEXTLINE(bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
        return e.code();
    }

    int correct = -1;
    const auto ec = chl_check_auth_credentials(*_comm, _input->username, _input->zone, _input->password, &correct);

    if (ec < 0) {
        return ec;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
    *_correct = static_cast<int*>(std::malloc(sizeof(int)));
    **_correct = correct;

    return ec;
} // rs_check_auth_credentials
