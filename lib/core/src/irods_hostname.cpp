#include <arpa/inet.h>
#include <ifaddrs.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <algorithm>
#include <cerrno>
#include <array>
#include <utility>
#include <iterator>
#include <vector>
#include <thread>
#include <chrono>

#include "irods_exception.hpp"
#include "rodsErrorTable.h"
#include "rodsLog.h"

bool
operator==(const struct in_addr& lhs, const struct in_addr& rhs) {
    return lhs.s_addr == rhs.s_addr;
}

bool
operator==(const struct in6_addr& lhs, const struct in6_addr& rhs) {
    return std::equal(std::begin(lhs.s6_addr), std::end(lhs.s6_addr), std::begin(rhs.s6_addr));
}

namespace {
    std::string
    to_string(const struct in_addr& address) {
        std::array<char, INET_ADDRSTRLEN> display_address;
        const char* ntop_ret = inet_ntop(AF_INET, &address, display_address.data(), sizeof(display_address));
        if (ntop_ret) {
            return ntop_ret;
        } else {
            std::stringstream s;
            s << "inet_ntop failed: " << errno;
            THROW( SYS_INTERNAL_ERR, s.str() );
        }
    }

    std::string
    to_string(const struct in6_addr& address) {
        std::array<char, INET6_ADDRSTRLEN> display_address;
        const char* ntop_ret = inet_ntop(AF_INET6, &address, display_address.data(), sizeof(display_address));
        if (ntop_ret) {
            return ntop_ret;
        } else {
            std::stringstream s;
            s << "inet_ntop failed: " << errno;
            THROW( SYS_INTERNAL_ERR, s.str() );
        }
    }



    bool
    is_loopback(const struct in_addr& v4_addr) {
        return (ntohl(v4_addr.s_addr) & 0xFF000000) == 0x7F000000;
    }

// as of 1.60, boost::asio::ip::address_v6::is_loopback() does not check for ipv4-mapped loopback
    bool
    is_loopback(const struct in6_addr& v6_addr) {
        const unsigned char (&addr_bytes)[16] = v6_addr.s6_addr;
        const bool is_v6_loopback =
            addr_bytes[ 0] == 0 && addr_bytes[ 1] == 0 && addr_bytes[ 2] == 0 && addr_bytes[ 3] == 0 &&
            addr_bytes[ 4] == 0 && addr_bytes[ 5] == 0 && addr_bytes[ 6] == 0 && addr_bytes[ 7] == 0 &&
            addr_bytes[ 8] == 0 && addr_bytes[ 9] == 0 && addr_bytes[10] == 0 && addr_bytes[11] == 0 &&
            addr_bytes[12] == 0 && addr_bytes[13] == 0 && addr_bytes[14] == 0 && addr_bytes[15] == 1;
        const bool is_v4_mapped_loopback =
            addr_bytes[ 0] == 0 && addr_bytes[ 1] == 0 && addr_bytes[ 2] == 0 && addr_bytes[ 3] == 0 &&
            addr_bytes[ 4] == 0 && addr_bytes[ 5] == 0 && addr_bytes[ 6] == 0 && addr_bytes[ 7] == 0 &&
            addr_bytes[ 8] == 0 && addr_bytes[ 9] == 0 && addr_bytes[10] == 255 && addr_bytes[11] == 255 &&
            addr_bytes[12] == 127;
        return is_v6_loopback || is_v4_mapped_loopback;
    }

    std::pair<std::vector<struct in_addr>, std::vector<struct in6_addr>>
                 get_local_ipv4_and_ipv6_addresses() {
        std::pair<std::vector<struct in_addr>, std::vector<struct in6_addr>> ret;

        struct ifaddrs* addrs;
        const int getifa_ret = getifaddrs(&addrs);
        if (getifa_ret == 0) {
            struct ifaddrs* tmp = addrs;
            while (tmp) {
                if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET) {
                    struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
                    ret.first.push_back(pAddr->sin_addr);
                } else if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET6) {
                    struct sockaddr_in6 *pAddr = (struct sockaddr_in6 *)tmp->ifa_addr;
                    ret.second.push_back(pAddr->sin6_addr);
                }
                tmp = tmp->ifa_next;
            }
            freeifaddrs(addrs);
        } else {
            std::stringstream s;
            s << "getifaddrs failed, errno: " << errno;
            THROW( SYS_INTERNAL_ERR, s.str() );
        }
        return ret;
    }

    void
    print_local_ipv4_and_ipv6_addresses(void) {
        auto ipv4_and_ipv6_addresses = get_local_ipv4_and_ipv6_addresses();
        for (const auto& v4_addr : ipv4_and_ipv6_addresses.first) {
            std::cout << to_string(v4_addr) << std::endl;
        }
        for (const auto& v6_addr : ipv4_and_ipv6_addresses.second) {
            std::cout << to_string(v6_addr) << std::endl;
        }
    }

    int
    getaddrinfo_with_retry(const char* node, const char* service,
                           const struct addrinfo* hints, struct addrinfo **res) {
        const int max_retry = 50;
        for (int i=0; i<max_retry; ++i) {
            const int gai_ret = getaddrinfo(node, service, hints, res);
            if (gai_ret != EAI_AGAIN) {
                return gai_ret;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return EAI_AGAIN;
    }

    std::pair<std::vector<struct in_addr>, std::vector<struct in6_addr>>
        get_remote_ipv4_and_ipv6_addresses(const char* hostname) {
        std::pair<std::vector<struct in_addr>, std::vector<struct in6_addr>> ret;

        struct addrinfo hints;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = 0;
        struct addrinfo *res;
        const int gai_ret = getaddrinfo_with_retry(hostname, nullptr, &hints, &res);
        if (gai_ret==0) {
            struct addrinfo* p = res;
            while (p) {
                if (p->ai_family == AF_INET) {
                    struct sockaddr_in* sinp = reinterpret_cast<struct sockaddr_in*>(p->ai_addr);
                    ret.first.push_back(sinp->sin_addr);
                } else if (p->ai_family == AF_INET6) {
                    struct sockaddr_in6* sin6p = reinterpret_cast<struct sockaddr_in6*>(p->ai_addr);
                    ret.second.push_back(sin6p->sin6_addr);
                } else {
                    rodsLog( LOG_ERROR, "unknown ai_family: %d", p->ai_family );
                }
                p = p->ai_next;
            }
            freeaddrinfo(res);
        } else {
            std::stringstream s;
            s << "gai error: " << gai_ret << " " << gai_strerror(gai_ret);
            THROW( SYS_INTERNAL_ERR, s.str() );
        }
        return ret;
    }

    void
    print_remote_ipv4_and_ipv6_addresses(const char* hostname) {
        auto ipv4_and_ipv6_addresses = get_remote_ipv4_and_ipv6_addresses(hostname);
        for (const auto& v4_addr : ipv4_and_ipv6_addresses.first) {
            std::cout << to_string(v4_addr) << std::endl;
        }

        for (const auto& v6_addr : ipv4_and_ipv6_addresses.second) {
            std::cout << to_string(v6_addr) << std::endl;
        }
    }

    template <typename T>
    bool resolves_local(const std::vector<T>& local_addresses, const std::vector<T>& remote_addresses) {
        for (const auto& remote_address : remote_addresses) {
            if (is_loopback(remote_address)) {
                return true;
            }
            if (std::find(local_addresses.cbegin(), local_addresses.cend(), remote_address) != local_addresses.cend()) {
                return true;
            }
        }
        return false;
    }
}

bool
hostname_resolves_to_local_address(const char* hostname) {
    const auto remote_addresses = get_remote_ipv4_and_ipv6_addresses(hostname);
    const auto local_addresses = get_local_ipv4_and_ipv6_addresses();
    if (resolves_local(local_addresses.first, remote_addresses.first)) {
        return true;
    }
    if (resolves_local(local_addresses.second, remote_addresses.second)) {
        return true;
    }
    return false;
}
