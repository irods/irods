//
// This client is for testing the GitHub issue at https://github.com/irods/irods/issues/8733.
//
// It connects to an iRODS server and then holds the connection open until
// the service account OS user sends the process a SIGINT.
//

#include "irods/client_connection.hpp"
#include "irods/rcMisc.h"
#include "irods/rodsClient.h"

#include <unistd.h>

#include <csignal>
#include <cstring>
#include <iostream>

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
volatile std::sig_atomic_t g_stop = 0;

auto print_usage_info() -> void;

int main(int _argc, char* _argv[])
{
    if (_argc != 2) {
        std::cerr << "ERROR: Incorrect number of arguments.\n\n";
        print_usage_info();
        return 1;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (std::strlen(_argv[1]) == 0) {
        std::cerr << "ERROR: APP_NAME_STRING is empty.\n\n";
        print_usage_info();
        return 1;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    set_ips_display_name(_argv[1]);
    load_client_api_plugins();
    irods::experimental::client_connection conn; // NOLINT(misc-const-correctness)

    // Configure signal handler.
    struct sigaction sa_intr; // NOLINT(cppcoreguidelines-pro-type-member-init)
    sigemptyset(&sa_intr.sa_mask);
    sa_intr.sa_flags = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    sa_intr.sa_handler = [](int) { g_stop = 1; };
    if (sigaction(SIGINT, &sa_intr, nullptr) == -1) {
        return 1;
    }

    // Wait for signal to stop.
    while (0 == g_stop) {
        pause();
    }

    return 0;
}

auto print_usage_info() -> void
{
    // clang-format off
    std::cout <<
R"(irods_test_issue_8733 - Utility for testing issue #8733

Usage: irods_test_issue_8733 APP_NAME_STRING

Connects to an iRODS server using the credentials from the local user's
account and holds the connection open until the SIGINT signal is received.

APP_NAME_STRING is the string used to identify this application in ips output.
)";
    // clang-format on
}
