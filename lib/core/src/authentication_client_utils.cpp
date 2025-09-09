#include "irods/authentication_client_utils.hpp"

#include <cstdio>
#include <iostream>
#include <string>

#include <fmt/format.h>

#include <termios.h>

namespace irods::authentication
{
    auto get_password_from_client_stdin() -> std::string
    {
        // clang-format off
        struct termios tty{};
        // clang-format on
        tcgetattr(STDIN_FILENO, &tty);
        const tcflag_t oldflag = tty.c_lflag;
        tty.c_lflag &= ~ECHO; // NOLINT(hicpp-signed-bitwise)
        if (0 != tcsetattr(STDIN_FILENO, TCSANOW, &tty)) {
            const int errsv = errno;
            fmt::print("WARNING: Error {} disabling echo mode. Password will be displayed in plaintext.\n", errsv);
        }
        std::string password;
        std::getline(std::cin, password);
        fmt::print("\n");
        tty.c_lflag = oldflag;
        if (0 != tcsetattr(STDIN_FILENO, TCSANOW, &tty)) {
            fmt::print("Error reinstating echo mode.\n");
        }
        return password;
    } // get_password_from_client_stdin
} // namespace irods::authentication
