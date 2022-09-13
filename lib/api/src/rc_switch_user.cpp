#include "irods/switch_user.h"

#include "irods/plugins/api/switch_user_types.h"
#include "irods/plugins/api/api_plugin_number.h"
#include "irods/procApiRequest.h"
#include "irods/rodsErrorTable.h"
#include "irods/stringOpr.h"

#include <cstring>
#include <exception>

auto rc_switch_user(RcComm* _comm, const char* _username, const char* _zone) -> int
{
    // NOLINTNEXTLINE(readability-implicit-bool-conversion)
    if (!_comm || !_username || !_zone) {
        return SYS_INVALID_INPUT_PARAM;
    }

    // clang-format off
    //
    // The use of "is_non_empty_string()" guarantees that the string passed to it is not empty
    // and does not exceed the size of the buffer. It also guarantees that the string is null
    // terminated.
    //
    // Accepting empty strings for either parameter is not allowed.
    // This is especially true for the "_zone" parameter because this function copies the
    // string pointed to by "_zone" into the RcComm. Copying an empty string into the RcComm's
    // rodsZone member variable makes the RcComm invalid.
    if (is_non_empty_string(_username, sizeof(SwitchUserInput::username)) == 0 ||
        is_non_empty_string(_zone, sizeof(SwitchUserInput::zone)) == 0)
    {
        return SYS_INVALID_INPUT_PARAM;
    }
    // clang-format on

    auto& client = _comm->clientUser;

    // clang-format off
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    //
    // Return immediately if the RcComm object already represents the target user.
    //
    // This is allowed because the RcComm only cares about the username and zone. If the RcComm
    // considered the user's type or authentication level (i.e. LOCAL_PRIV_USER_AUTH), then this
    // optimization would not be possible.
    if (std::strncmp(_username, client.userName, sizeof(SwitchUserInput::username) - 1) == 0 &&
        std::strncmp(_zone, client.rodsZone, sizeof(SwitchUserInput::zone) - 1) == 0)
    {
        return 0;
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    // clang-format on

    try {
        SwitchUserInput input{};
        std::strncpy(static_cast<char*>(input.username), _username, sizeof(SwitchUserInput::username));
        std::strncpy(static_cast<char*>(input.zone), _zone, sizeof(SwitchUserInput::zone));

        const int ec = procApiRequest(_comm, SWITCH_USER_APN, &input, nullptr, nullptr, nullptr);

        if (ec == 0) {
            // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            std::strncpy(client.userName, _username, sizeof(UserInfo::userName));
            std::strncpy(client.rodsZone, _zone, sizeof(UserInfo::rodsZone));
            // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        }

        return ec;
    }
    catch (const std::exception&) {
        return SYS_LIBRARY_ERROR;
    }
    catch (...) {
        return SYS_UNKNOWN_ERROR;
    }
} // rc_switch_user
