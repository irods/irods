#include "irods/irods_service_account.hpp"

#include "irods/irods_exception.hpp"
#include "irods/irods_get_full_path_for_config_file.hpp"
#include "irods/irods_kvp_string_parser.hpp"
#include "irods/irods_log.hpp"
#include "irods/rodsErrorTable.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <memory>

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include <boost/algorithm/string.hpp>
#include <fmt/format.h>

namespace irods
{
    namespace
    {
        const std::string SERVICE_ACCOUNT_CONFIG_FILE("service_account.config");
        const std::string SERVICE_ACCOUNT_KW("IRODS_SERVICE_ACCOUNT_NAME");

        const std::size_t PWD_BUF_LEN_DEFAULT = 16384; // default size of buffer for strings in struct passwd
        const int PWD_BUF_LEN_MINIMUM = 128; // minimum size of buffer for strings in struct passwd
    } //namespace

    // FIXME: Parsing of service_account.config isn't super robust.
    auto is_service_account() -> bool
    {
        std::string svc_acct_cfg_fn;
        irods::error ret = irods::get_full_path_for_config_file(SERVICE_ACCOUNT_CONFIG_FILE, svc_acct_cfg_fn);
        if (!ret.ok()) {
            // if config file not found, the machine probably does not have a server
            return false;
        }

        std::ifstream svc_acct_cfg_strm{svc_acct_cfg_fn};
        if (!svc_acct_cfg_strm.is_open()) {
            // if we can't open the config file, it's likely due to a permission issue
            // meaning it is extremely unlikely we are the service account
            return false;
        }

        kvp_map_t service_acct_cfg;
        std::uint64_t cfg_line_no = 0;
        for (std::string cfg_line; std::getline(svc_acct_cfg_strm, cfg_line);) {
            cfg_line_no++;
            boost::algorithm::trim(cfg_line); // trim whitespace (should handle CRLF)
            if (cfg_line.empty()) {
                // ignore empty lines
                continue;
            }
            if (cfg_line.at(0) == '#') {
                // ignore lines starting with #
                continue;
            }

            // For now, let's just re-use our kvp-parsing functionality
            // FIXME: does not handle spaces around "=", quotes, inline comments, or escape-newline
            ret = parse_escaped_kvp_string(cfg_line, service_acct_cfg);
            if (!ret.ok()) {
                irods::log(LOG_WARNING,
                           fmt::format("{} - {} parse failure on line {}",
                                       __PRETTY_FUNCTION__,
                                       SERVICE_ACCOUNT_CONFIG_FILE,
                                       cfg_line_no));
            }
        }

        const auto svc_acct_itr = service_acct_cfg.find(SERVICE_ACCOUNT_KW);
        if (svc_acct_itr == service_acct_cfg.end()) {
            THROW(
                KEY_NOT_FOUND, fmt::format("could not find {} in {}", SERVICE_ACCOUNT_KW, SERVICE_ACCOUNT_CONFIG_FILE));
        }
        const std::string service_account_username = svc_acct_itr->second;

        // compare uids rather than usernames, to account for overlapping uids
        // NOTE: do not reference the strings inside these structs, as their buffer goes out of scope
        struct passwd svc_acct_pwd{};
        struct passwd* svc_acct_pwdp = nullptr;
        const auto _pwd_init_len = sysconf(_SC_GETPW_R_SIZE_MAX);
        std::size_t pwd_buf_len =
            PWD_BUF_LEN_DEFAULT; // a reasonable buffer size in case sysconf gives us something awful
        if (_pwd_init_len >= PWD_BUF_LEN_MINIMUM) {
            pwd_buf_len = static_cast<std::size_t>(_pwd_init_len);
        }
        int pwd_e = 0;
        while (true) {
            // this sure is a lot of fuss to allocate a buffer for data we're not going to use anyway...
            const std::unique_ptr<char[]> pwd_buf( // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
                new char[pwd_buf_len]);
            pwd_e =
                getpwnam_r(service_account_username.c_str(), &svc_acct_pwd, pwd_buf.get(), pwd_buf_len, &svc_acct_pwdp);
            if (pwd_e != ERANGE) {
                break;
            }
            // buffer wasn't big enough, try again
            const size_t new_len = pwd_buf_len * 2;
            // check for overflow
            // FIXME: compilers can optimize away this check
            if (pwd_buf_len > new_len) {
                THROW(SYS_MALLOC_ERR, "integer overflow when allocating space for passwd struct data");
            }
            pwd_buf_len = new_len;
        }
        switch (pwd_e) {
            case 0:
                break;
            case EMFILE:
            case ENFILE:
                THROW(SYS_OUT_OF_FILE_DESC - pwd_e, "hit file descriptor limit");
                break;
            case EINTR:
                THROW(SYS_CAUGHT_SIGNAL - pwd_e, "caught signal");
                break;
            default:
                THROW(SYS_UNKNOWN_ERROR - pwd_e, "unknown error when calling getpwnam_r");
                break;
        }
        if (svc_acct_pwdp == nullptr) {
            irods::log(
                LOG_WARNING,
                fmt::format("{} - could not find user with name {}", __PRETTY_FUNCTION__, service_account_username));
            return false;
        }
        return svc_acct_pwd.pw_uid == getuid();
    }
} //namespace irods
