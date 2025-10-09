#include "irods/authentication_client_utils.hpp"

#include "irods/irods_exception.hpp"
#include "irods/rodsErrorTable.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <fmt/format.h>

#include <termios.h>

namespace
{
    auto get_session_token_file_path() -> std::filesystem::path
    {
        // See whether the environment has a variable which specifies where the session token file is located.
        constexpr const char* session_token_file_env_var = "IRODS_SESSION_TOKEN_FILE_PATH";
        const char* env_var = std::getenv(session_token_file_env_var);
        if (nullptr != env_var && '\0' != *env_var) {
            return std::filesystem::path{env_var};
        }

        // If no HOME environment variable is set, this is not an error. We just don't know where the session token
        // file is located, so we must return nothing.
        const char* home_var = std::getenv("HOME");
        if (nullptr == home_var) {
            return {};
        }

        constexpr const char* session_token_filename_default = ".irods/.irods_secrets";
        return std::filesystem::path{home_var} / session_token_filename_default;
    } // get_session_token_file_path
} // anonymous namespace

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

    auto read_session_token_from_file() -> std::string
    {
        const auto session_token_file_path = get_session_token_file_path();
        if (!std::filesystem::exists(session_token_file_path)) {
            return {};
        }

        std::ifstream session_token_file_stream{session_token_file_path.c_str()};
        if (!session_token_file_stream.is_open()) {
            const auto irods_error_code = UNIX_FILE_OPEN_ERR;
            THROW(irods_error_code,
                  fmt::format("Failed to open session token file [{}].", session_token_file_path.c_str()));
        }

        std::string session_token_file_contents;
        session_token_file_stream >> session_token_file_contents;
        session_token_file_stream.close();
        if (session_token_file_contents.size() != session_token_length) {
            THROW(SYS_INVALID_INPUT_PARAM,
                  fmt::format("Session token expected to be length [{}], but had length [{}].",
                              session_token_length,
                              session_token_file_contents.size()));
        }
        return session_token_file_contents;
    } // read_session_token_from_file

    auto write_session_token_to_file(const std::string& _session_token) -> void
    {
        if (_session_token.size() != session_token_length) {
            THROW(
                SYS_INVALID_INPUT_PARAM,
                fmt::format("Session token must be a 36-character UUID, but had length [{}].", _session_token.size()));
        }

        const auto session_token_file_path = get_session_token_file_path();
        std::ofstream session_token_file_stream{session_token_file_path.c_str()};
        if (!session_token_file_stream.is_open()) {
            const auto irods_error_code = UNIX_FILE_OPEN_ERR;
            THROW(irods_error_code,
                  fmt::format("Failed to open session token file [{}].", session_token_file_path.c_str()));
        }

        session_token_file_stream << _session_token;
        session_token_file_stream.close();
        // Make sure that only the owner can read/write this file because it contains sensitive information.
        std::filesystem::permissions(
            session_token_file_path.c_str(), std::filesystem::perms::owner_read | std::filesystem::perms::owner_write);
    } // write_session_token_to_file

    auto remove_session_token_file() -> int
    {
        try {
            if (!std::filesystem::remove(get_session_token_file_path())) {
                return static_cast<int>(UNIX_FILE_UNLINK_ERR);
            }
            return 0;
        }
        catch (const std::filesystem::filesystem_error& e) {
            return static_cast<int>(UNIX_FILE_UNLINK_ERR + e.code().value());
        }
        catch (const std::exception& e) {
            return static_cast<int>(SYS_LIBRARY_ERROR);
        }
    } // remove_session_token_file
} // namespace irods::authentication
