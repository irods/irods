#include "irods/user_administration.hpp"

#include "irods/obf.h"
#include "irods/authenticate.h" // For MAX_PASSWORD_LEN
#include "irods/rodsErrorTable.h"
#include "irods/irods_exception.hpp"

#include <fmt/format.h>

#include <cstring>
#include <array>
#include <string>

namespace irods::experimental::administration::detail
{
    auto obfuscate_password(const user_password_property& _property) -> std::string
    {
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        std::array<char, MAX_PASSWORD_LEN + 10> plain_text_password{};
        std::strncpy(plain_text_password.data(), _property.value.data(), MAX_PASSWORD_LEN);

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        if (const auto count = MAX_PASSWORD_LEN - 10 - _property.value.size(); count > 15) {
            // The random string (second argument) is used for padding and must match
            // what is defined on the server-side.
            std::strncat(plain_text_password.data(), "1gCBizHWbwIYyWLoysGzTe6SyzqFKMniZX05faZHWAwQKXf6Fs", count);
        }

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        std::array<char, MAX_PASSWORD_LEN + 10> key{};

        if (_property.requester_password) {
            if (_property.requester_password->size() >= MAX_PASSWORD_LEN) {
                constexpr const char* msg_fmt = "Password key cannot exceed maximum size [{}].";
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format(msg_fmt, MAX_PASSWORD_LEN));
            }

            std::strncpy(key.data(), _property.requester_password->c_str(), MAX_PASSWORD_LEN);
        }
        else {
            // Get the plain text password of the iRODS user.
            // "obfGetPw" decodes the obfuscated password stored in .irods/irodsA.
            if (const auto ec = obfGetPw(key.data()); ec != 0) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                THROW(ec, "Failed to decode obfuscated password in .irodsA file.");
            }
        }

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        std::array<char, MAX_PASSWORD_LEN + 100> obfuscated_password{};
        obfEncodeByKey(plain_text_password.data(), key.data(), obfuscated_password.data());

        return obfuscated_password.data();
    } // obfuscate_password
} // namespace irods::experimental::administration::detail
