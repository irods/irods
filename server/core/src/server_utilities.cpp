#include "server_utilities.hpp"

#include "dataObjInpOut.h"
#include "key_value_proxy.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

#include <regex>

namespace irods
{
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
} // namespace irods

