#include "irods/server_utilities.hpp"

#include "irods/dataObjInpOut.h"
#include "irods/key_value_proxy.hpp"
#include "irods/irods_logger.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "irods/filesystem.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <boost/filesystem.hpp>

#include <regex>
#include <fstream>

namespace irods
{
    // clang-format off
    const std::string_view PID_FILENAME_MAIN_SERVER  = "irods.pid";
    const std::string_view PID_FILENAME_DELAY_SERVER = "irods_delay_server.pid";
    // clang-format on

    auto is_force_flag_required(RsComm& _comm, const DataObjInp& _input) -> bool
    {
        namespace ix = irods::experimental;
        namespace fs = irods::experimental::filesystem;

        return fs::server::exists(_comm, _input.objPath) &&
               !ix::key_value_proxy{_input.condInput}.contains(FORCE_FLAG_KW);
    }

    auto contains_session_variables(const std::string_view _rule_text) -> bool
    {
        static const std::regex session_vars_pattern{
            "\\$\\b(?:pluginInstanceName|status|objPath|objPath|dataType|dataSize|dataSize|chksum|"
            "version|filePath|replNum|replStatus|writeFlag|dataOwner|dataOwnerZone|"
            "dataExpiry|dataComments|dataCreate|dataModify|dataAccess|dataAccessInx|"
            "dataId|collId|statusString|destRescName|backupRescName|rescName|userClient|"
            "userClient|userNameClient|userNameClient|rodsZoneClient|rodsZoneClient|sysUidClient|sysUidClient|"
            "privClient|privClient|clientAddr|authStrClient|authStrClient|userAuthSchemeClient|userAuthSchemeClient|userInfoClient|"
            "userInfoClient|userCommentClient|userCommentClient|userCreateClient|userCreateClient|userModifyClient|userModifyClient|userProxy|"
            "userProxy|userNameProxy|userNameProxy|rodsZoneProxy|rodsZoneProxy|privProxy|privProxy|authStrProxy|"
            "authStrProxy|userAuthSchemeProxy|userAuthSchemeProxy|userInfoProxy|userInfoProxy|userCommentProxy|userCommentProxy|"
            "userCreateProxy|userCreateProxy|userModifyProxy|userModifyProxy|collName|collParentName|"
            "collOwnername|collExpiry|collComments|collCreate|collModify|collAccess|"
            "collAccessInx|collInheritance|otherUser|otherUserName|otherUserZone|otherUserType|"
            "connectCnt|connectSock|connectOption|connectStatus|connectApiInx|KVPairs)\\b"
        };

        return std::regex_search(_rule_text.data(), session_vars_pattern);
    }

    auto to_bytes_buffer(const std::string_view _s) -> BytesBuf*
    {
        constexpr auto allocate = [](const auto bytes) noexcept
        {
            return std::memset(std::malloc(bytes), 0, bytes);
        };

        const auto buf_size = _s.length() + 1;

        auto* buf = static_cast<char*>(allocate(sizeof(char) * buf_size));
        std::strncpy(buf, _s.data(), _s.length());

        auto* bbp = static_cast<BytesBuf*>(allocate(sizeof(BytesBuf)));
        bbp->len = buf_size;
        bbp->buf = buf;

        return bbp;
    } // to_bytes_buffer

    auto create_pid_file(const std::string_view _pid_filename) -> int
    {
        using log = irods::experimental::log;

        const auto pid_file = boost::filesystem::temp_directory_path() / _pid_filename.data();

        // Open the PID file. If it does not exist, create it and give the owner
        // permission to read and write to it.
        const auto fd = open(pid_file.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if (fd == -1) {
            log::server::error("Could not open PID file.");
            return -1;
        }

        // Get the current open flags for the open file descriptor.
        const auto flags = fcntl(fd, F_GETFD);
        if (flags == -1) {
            log::server::error("Could not retrieve open flags for PID file.");
            return -1;
        }

        // Enable the FD_CLOEXEC option for the open file descriptor.
        // This option will cause successful calls to exec() to close the file descriptor.
        // Keep in mind that record locks are NOT inherited by forked child processes.
        if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1) {
            log::server::error("Could not set FD_CLOEXEC on PID file.");
            return -1;
        }

        struct flock input;
        input.l_type = F_WRLCK;
        input.l_whence = SEEK_SET;
        input.l_start = 0;
        input.l_len = 0;

        // Try to acquire the write lock on the PID file. If we cannot get the lock,
        // another instance of the application must already be running or something
        // weird is going on.
        if (fcntl(fd, F_SETLK, &input) == -1) {
            if (EAGAIN == errno || EACCES == errno) {
                log::server::error("Could not acquire write lock for PID file. Another instance "
                                   "could be running already.");
                return -1;
            }
        }
        
        if (ftruncate(fd, 0) == -1) {
            log::server::error("Could not truncate PID file's contents.");
            return -1;
        }

        const auto contents = fmt::format("{}\n", getpid());
        if (write(fd, contents.data(), contents.size()) != static_cast<long>(contents.size())) {
            log::server::error("Could not write PID to PID file.");
            return -1;
        }

        return 0;
    } // create_pid_file

    std::optional<pid_t> get_pid_from_file(const std::string_view _pid_filename) noexcept
    {
        using log = irods::experimental::log::server;

        try {
            const auto pid_file = boost::filesystem::temp_directory_path() / _pid_filename.data();

            if (!boost::filesystem::exists(pid_file)) {
                log::trace("PID file does not exist [path={}].", pid_file.c_str());
                return std::nullopt;
            }

            pid_t pid;

            if (!(std::ifstream{pid_file.c_str()} >> pid)) {
                log::trace("Could not retrieve PID from file [path={}].", pid_file.c_str());
                return std::nullopt;
            }

            // The process is running if waitpid() returns a value other than -1. If -1
            // is returned and errno is set to ECHILD, that means the process specified
            // by "pid" does not exist or is not a child of the calling process.
            if (waitpid(pid, nullptr, WNOHANG) != -1) {
                return pid;
            }
        }
        catch (const std::exception& e) {
            log::error("Caught exception in get_pid_from_file() [_pid_filename={}]: {}",
                       _pid_filename, e.what());
        }
        catch (...) {
            log::error("Caught unknown exception in get_pid_from_file() [_pid_filename={}].",
                       _pid_filename);
        }

        return std::nullopt;
    } // get_pid_from_file
} // namespace irods

