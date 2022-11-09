#include "irods/user_administration.hpp"

#include "irods/obf.h"
#include "irods/authenticate.h" // For MAX_PASSWORD_LEN
#include "irods/rodsErrorTable.h"
#include "irods/irods_exception.hpp"

#include <cstring>
#include <array>
#include <string>
#include <string_view>

namespace irods::experimental::administration::detail
{
    auto obfuscate_password(const std::string_view _new_password) -> std::string
    {
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        std::array<char, MAX_PASSWORD_LEN + 10> plain_text_password{};
        std::strncpy(plain_text_password.data(), _new_password.data(), MAX_PASSWORD_LEN);

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        if (const auto lcopy = MAX_PASSWORD_LEN - 10 - _new_password.size(); lcopy > 15) {
            // The random string (second argument) is used for padding and must match
            // what is defined on the server-side.
            std::strncat(plain_text_password.data(), "1gCBizHWbwIYyWLoysGzTe6SyzqFKMniZX05faZHWAwQKXf6Fs", lcopy);
        }

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        std::array<char, MAX_PASSWORD_LEN + 10> admin_password{};

        // Get the plain text password of the iRODS user.
        // "obfGetPw" decodes the obfuscated password stored in .irods/irodsA.
        if (const auto ec = obfGetPw(admin_password.data()); ec != 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, "Failed to decode obfuscated password in .irodsA file.");
        }

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        std::array<char, MAX_PASSWORD_LEN + 100> obfuscated_password{};
        obfEncodeByKeyV2(
            plain_text_password.data(),
            admin_password.data(),
            getSessionSignatureClientside(),
            obfuscated_password.data());

        return obfuscated_password.data();
    } // obfuscate_password
} // namespace irods::experimental::administration::detail
