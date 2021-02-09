#include "dns_cache.hpp"

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/named_sharable_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>

#include <cstring>
#include <utility>
#include <algorithm>

#include <sys/types.h>
#include <unistd.h>

namespace
{
    namespace bi = boost::interprocess;

    using std::chrono::duration_cast;
    using std::chrono::seconds;

    struct address_info;

    // clang-format off
    using segment_manager_type         = bi::managed_shared_memory::segment_manager;
    using void_allocator_type          = bi::allocator<void, segment_manager_type>;
    using char_allocator_type          = bi::allocator<char, segment_manager_type>;
    using address_info_allocator_type  = bi::allocator<address_info, segment_manager_type>;
    using key_type                     = bi::basic_string<char, std::char_traits<char>, char_allocator_type>;
    using mapped_type                  = bi::list<address_info, address_info_allocator_type>;
    using value_type                   = std::pair<const key_type, mapped_type>;
    using value_allocator_type         = bi::allocator<value_type, segment_manager_type>;
    using map_type                     = bi::map<key_type, mapped_type, std::less<key_type>, value_allocator_type>;
    using clock_type                   = std::chrono::system_clock;
    // clang-format on

    //
    // Global Variables
    //

    // The following variables define the names of shared memory objects and other properties.
    std::string g_segment_name;
    std::size_t g_segment_size;
    std::string g_mutex_name;

    // On initialization, holds the PID of the process that initialized the hostname cache.
    // This ensures that only the process that initialized the system can deinitialize it.
    pid_t g_owner_pid;

    // The following are pointers to the shared memory objects and allocator.
    // Allocating on the heap allows us to know when the hostname cache is constructed/destructed.
    std::unique_ptr<bi::managed_shared_memory> g_segment;
    std::unique_ptr<void_allocator_type> g_allocator;
    std::unique_ptr<bi::named_sharable_mutex> g_mutex;
    map_type* g_map;

    // The shared memory type of addrinfo.
    struct address_info
    {
        address_info(const addrinfo& _info, seconds _expires_after)
            : flags{_info.ai_flags}
            , family{_info.ai_family}
            , socktype{_info.ai_socktype}
            , protocol{_info.ai_protocol}
            , addrlen{_info.ai_addrlen}
            , addr{}
            , canonname{}
            , expiration{}
            , expires_after{_expires_after.count()}
        {
            if (_info.ai_addr) {
                addr = static_cast<sockaddr*>(g_segment->allocate(sizeof(sockaddr)));
                std::memcpy(addr.get(), _info.ai_addr, sizeof(sockaddr));
            }

            if (_info.ai_canonname) {
                const auto size = std::strlen(_info.ai_canonname);
                canonname = static_cast<char*>(g_segment->allocate(sizeof(char) * size + 1));
                std::strncpy(canonname.get(), _info.ai_canonname, size);
                canonname[size] = 0;
            }

            const auto tp = clock_type::now() + _expires_after;
            expiration = duration_cast<seconds>(tp.time_since_epoch()).count();
        }

        address_info(const address_info&) = default;
        auto operator=(const address_info&) -> address_info& = default;

        address_info(address_info&&) = default;
        auto operator=(address_info&&) -> address_info& = default;

        ~address_info()
        {
            if (!g_segment) {
                return;
            }

            try {
                // clang-format off
                if (addr)      { g_segment->deallocate(addr.get()); }
                if (canonname) { g_segment->deallocate(canonname.get()); }
                // clang-format on
            }
            catch (...) {}
        }

        auto to_addrinfo() const -> addrinfo*
        {
            auto* p = static_cast<addrinfo*>(std::malloc(sizeof(addrinfo)));
            std::memset(p, 0, sizeof(addrinfo));

            // clang-format off
            p->ai_flags     = flags;
            p->ai_family    = family;
            p->ai_socktype  = socktype;
            p->ai_protocol  = protocol;
            p->ai_addrlen   = addrlen;
            // clang-format on

            if (addr) {
                p->ai_addr = static_cast<sockaddr*>(std::malloc(sizeof(sockaddr)));
                std::memcpy(p->ai_addr, addr.get(), sizeof(sockaddr));
            }

            if (canonname) {
                const auto size = std::strlen(canonname.get());
                p->ai_canonname = static_cast<char*>(std::malloc(sizeof(char) * size + 1));
                std::strncpy(p->ai_canonname, canonname.get(), size);
                p->ai_canonname[size] = 0;
            }

            return p;
        }

        int flags;
        int family;
        int socktype;
        int protocol;
        socklen_t addrlen;
        bi::offset_ptr<sockaddr> addr;
        bi::offset_ptr<char> canonname;
        std::int64_t expiration;
        std::int64_t expires_after;
    }; // struct address_info

    auto free_address_info(addrinfo* _p) -> void
    {
        for (addrinfo* current = _p, *prev = nullptr; current;) {
            if (current->ai_addr)      { std::free(current->ai_addr); }
            if (current->ai_canonname) { std::free(current->ai_canonname); }

            prev = current;
            current = current->ai_next;

            std::free(prev);
        }
    }

    auto current_timestamp_in_seconds() noexcept -> std::int64_t
    {
        return duration_cast<seconds>(clock_type::now().time_since_epoch()).count();
    }
} // anonymous namespace

namespace irods::experimental::net::dns_cache
{
    auto init(const std::string_view _shm_name, std::size_t _shm_size) -> void
    {
        if (getpid() == g_owner_pid) {
            return;
        }

        g_segment_name = _shm_name.data();
        g_segment_size = _shm_size;
        g_mutex_name = g_segment_name + "_mutex";

        bi::named_sharable_mutex::remove(g_mutex_name.data());
        bi::shared_memory_object::remove(g_segment_name.data());

        g_owner_pid = getpid();
        g_segment = std::make_unique<bi::managed_shared_memory>(bi::create_only, g_segment_name.data(), g_segment_size);
        g_allocator = std::make_unique<void_allocator_type>(g_segment->get_segment_manager());
        g_mutex = std::make_unique<bi::named_sharable_mutex>(bi::create_only, g_mutex_name.data());
        g_map = g_segment->construct<map_type>(bi::anonymous_instance)(std::less<key_type>{}, *g_allocator);
    } // init

    auto deinit() noexcept -> void
    {
        if (getpid() != g_owner_pid) {
            return;
        }

        try {
            g_owner_pid = 0;

            if (g_segment && g_map) {
                g_segment->destroy_ptr(g_map);
                g_map = nullptr;
            }

            // clang-format off
            if (g_mutex)     { g_mutex.reset(); }
            if (g_allocator) { g_allocator.reset(); }
            if (g_segment)   { g_segment.reset(); }
            // clang-format on

            bi::named_sharable_mutex::remove(g_mutex_name.data());
            bi::shared_memory_object::remove(g_segment_name.data());
        }
        catch (...) {}
    } // deinit

    auto insert_or_assign(const std::string_view _key,
                          const addrinfo& _info,
                          seconds _expires_after) -> bool
    {
        void_allocator_type alloc{g_segment->get_segment_manager()};

        bi::scoped_lock lk{*g_mutex};

        auto [iter, inserted] = g_map->insert_or_assign(key_type{_key.data(), *g_allocator}, mapped_type{alloc});

        for (const auto* p = &_info; p; p = p->ai_next) {
            iter->second.emplace_back(*p, _expires_after);
        }

        return inserted;
    } // insert_or_assign

    auto lookup(const std::string_view _key) -> std::unique_ptr<addrinfo, addrinfo_deleter_type>
    {
        bi::sharable_lock lk{*g_mutex};

        if (auto iter = g_map->find(key_type{_key.data(), *g_allocator}); iter != g_map->end()) {
            if (auto& list = iter->second; current_timestamp_in_seconds() < list.front().expiration) {
                addrinfo* first{};
                addrinfo* prev{};
                addrinfo* current{};

                for (auto& node : list) {
                    current = node.to_addrinfo();

                    if (!prev) {
                        first = current;
                    }
                    else {
                        prev->ai_next = current;
                    }

                    prev = current;
                }

                // Not bumping the expiration timestamp here means the entry will eventually expire
                // and cause a cache miss which is totally fine.

                return {first, free_address_info};
            }
        }

        return {nullptr, nullptr};
    } // lookup

    auto erase(const std::string_view _key) -> void
    {
        bi::scoped_lock lk{*g_mutex};
        g_map->erase(key_type{_key.data(), *g_allocator});
    } // erase

    auto erase_expired_entries() -> void
    {
        bi::scoped_lock lk{*g_mutex};

        const auto now = current_timestamp_in_seconds();

        for (auto iter = g_map->begin(), end = g_map->end(); iter != end;) {
            if (now >= iter->second.front().expiration) {
                iter = g_map->erase(iter);
            }
            else {
                ++iter;
            }
        }
    } // erase_expired_entries

    auto clear() -> void
    {
        bi::scoped_lock lk{*g_mutex};
        g_map->clear();
    } // clear

    auto size() -> std::size_t
    {
        bi::sharable_lock lk{*g_mutex};
        return g_map->size();
    } // size

    auto available_memory() -> std::size_t
    {
        bi::sharable_lock lk{*g_mutex};
        return g_segment->get_free_memory();
    } // available_memory
} // namespace irods::experimental::net::dns_cache

