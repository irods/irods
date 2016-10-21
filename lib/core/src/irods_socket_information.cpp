#include <algorithm>
#include <arpa/inet.h>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <vector>

#include "rodsErrorTable.h"
#include "irods_exception.hpp"


std::string socket_fd_to_remote_address(const int fd) {
    struct sockaddr_storage addr;
    socklen_t addr_size = sizeof(addr);
    const int getpeername_result = getpeername(fd, (struct sockaddr *)&addr, &addr_size); // 0 or -1 and errno
    if (getpeername_result != 0) {
        if (errno == ENOTCONN) {
            return "";
        }
        THROW(SYS_INTERNAL_ERR, boost::format("getpeername of fd [%d] failed with errno [%d]") % fd % errno);
    }

    char ipstr[INET6_ADDRSTRLEN] = {};
    if (addr.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&addr;
        inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof(ipstr));
    } else if (addr.ss_family == AF_INET6) {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
        inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof(ipstr));
    }

    return ipstr;
}

std::vector<int> get_open_socket_file_descriptors(void) {
    std::vector<int> ret;
    try {
        const boost::filesystem::directory_iterator end;
        boost::filesystem::directory_iterator it("/proc/self/fd");
        for ( ; it != end; ++it) {
            if (boost::filesystem::is_symlink(it->path())) {
                boost::filesystem::path symlink_contents = boost::filesystem::read_symlink(it->path());
                if (!symlink_contents.string().compare(0, 8, "socket:[")) {
                    ret.push_back(atoi(it->path().filename().string().c_str()));
                }
            }
        }
    } catch (const boost::filesystem::filesystem_error& e) {
        THROW(SYS_INTERNAL_ERR, boost::format("boost::filesystem error [%s]") % e.what());
    }

    return ret;
}
