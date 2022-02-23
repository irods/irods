#include "irods/replica_access_table.hpp"

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/sync/named_upgradable_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/upgradable_lock.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <fmt/format.h>

#include <memory>
#include <algorithm>
#include <vector>
#include <chrono>

namespace irods::experimental::replica_access_table
{
    namespace
    {
        namespace bi = boost::interprocess;

        // clang-format off
        using segment_manager_type  = bi::managed_shared_memory::segment_manager;
        using void_allocator_type   = bi::allocator<void, segment_manager_type>;
        // clang-format on

        // The value type mapped to a specific replica token.
        struct access_entry
        {
            // clang-format off
            using value_type     = pid_t;
            using allocator_type = bi::allocator<value_type, segment_manager_type>;
            using container_type = bi::vector<value_type, allocator_type>;
            // clang-format on

            data_id_type data_id;
            replica_number_type replica_number;
            container_type agent_pids;
        }; // struct access_entry

        // clang-format off
        using char_allocator_type  = bi::allocator<char, segment_manager_type>;
        using key_type             = bi::basic_string<char, std::char_traits<char>, char_allocator_type>;
        using mapped_type          = access_entry;
        using value_type           = std::pair<const key_type, mapped_type>;
        using value_allocator_type = bi::allocator<value_type, segment_manager_type>;
        using map_type             = bi::map<key_type, mapped_type, std::less<key_type>, value_allocator_type>;
        // clang-format on

        //
        // Global Variables
        //

        // The following variables define the names of shared memory objects and other properties.
        std::string g_segment_name;
        std::size_t g_segment_size;
        std::string g_mutex_name;

        // On initialization, holds the PID of the process that initialized the replica access table.
        // This ensures that only the process that initialized the system can deinitialize it.
        pid_t g_owner_pid;

        // The following are pointers to the shared memory objects and allocator.
        // Allocating on the heap allows us to know when the replica access table is constructed/destructed.
        std::unique_ptr<bi::managed_shared_memory> g_segment;
        std::unique_ptr<void_allocator_type> g_allocator;
        std::unique_ptr<bi::named_upgradable_mutex> g_mutex;
        map_type* g_map;

        // Returns a new replica token (i.e. UUID) allocated in shared memory.
        auto generate_replica_token() -> key_type
        {
            key_type uuid{*g_allocator};
            uuid.reserve(36);
            uuid = to_string(boost::uuids::random_generator{}()).data();

            while (g_map->find(uuid) != g_map->end()) {
                uuid = to_string(boost::uuids::random_generator{}()).data();
            }

            return uuid;
        } // generate_replica_token

        auto current_timestamp_in_seconds() noexcept -> std::int64_t
        {
            using std::chrono::system_clock;
            using std::chrono::seconds;
            using std::chrono::duration_cast;

            return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
        }
    } // anonymous namespace

    auto init(const std::string_view _shm_name, std::size_t _shm_size) -> void
    {
        if (getpid() == g_owner_pid) {
            return;
        }

        g_segment_name = fmt::format("{}_{}_{}", _shm_name, getpid(), current_timestamp_in_seconds());
        g_segment_size = _shm_size;
        g_mutex_name = g_segment_name + "_mutex";

        bi::named_upgradable_mutex::remove(g_mutex_name.data());
        bi::shared_memory_object::remove(g_segment_name.data());

        g_owner_pid = getpid();
        g_segment = std::make_unique<bi::managed_shared_memory>(bi::create_only, g_segment_name.data(), g_segment_size);
        g_allocator = std::make_unique<void_allocator_type>(g_segment->get_segment_manager());
        g_mutex = std::make_unique<bi::named_upgradable_mutex>(bi::create_only, g_mutex_name.data());
        g_map = g_segment->construct<map_type>(bi::anonymous_instance)(std::less<key_type>{}, *g_allocator);
    } // init

    auto deinit() noexcept -> void
    {
        // Only allow the process that called init() to remove the shared memory.
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

            bi::named_upgradable_mutex::remove(g_mutex_name.data());
            bi::shared_memory_object::remove(g_segment_name.data());
        }
        catch (...) {}
    } // deinit

    auto create_new_entry(data_id_type _data_id,
                          replica_number_type _replica_number,
                          pid_t _pid) -> replica_token_type
    {
        bi::scoped_lock lk{*g_mutex};

        const auto end = g_map->end();
        const auto exists = [_data_id, _replica_number](const value_type& v)
        {
            return v.second.data_id == _data_id && v.second.replica_number == _replica_number;
        };

        if (auto iter = std::find_if(g_map->begin(), end, exists); iter != end) {
            throw replica_access_table_error{"replica_access_table: Entry already exists"};
        }

        auto uuid = generate_replica_token();
        mapped_type v{_data_id, _replica_number, access_entry::container_type{{_pid}, *g_allocator}};
        g_map->insert(value_type{uuid, v});

        return uuid.data();
    } // create_new_entry

    auto append_pid(replica_token_view_type _token,
                    data_id_type _data_id,
                    replica_number_type _replica_number,
                    pid_t _pid) -> void
    {
        bi::scoped_lock lk{*g_mutex};

        auto iter = g_map->find(key_type{_token.data(), *g_allocator});

        if (iter == g_map->end()) {
            throw replica_access_table_error{"replica_access_table: Invalid token"};
        }

        auto& [k, v] = *iter;

        if (v.data_id != _data_id || v.replica_number != _replica_number) {
            throw replica_access_table_error{"replica_access_table: Invalid data id or replica number"};
        }

        v.agent_pids.push_back(_pid);
    } // append_pid

    auto contains(data_id_type _data_id, replica_number_type _replica_number) -> bool
    {
        bi::sharable_lock lk{*g_mutex};

        for (auto&& [k, v] : *g_map) {
            if (v.data_id == _data_id && v.replica_number == _replica_number) {
                return true;
            }
        }

        return false;
    } // contains

    auto contains(replica_token_view_type _token,
                      data_id_type _data_id,
                      replica_number_type _replica_number) -> bool
    {
        bi::sharable_lock lk{*g_mutex};

        if (const auto iter = g_map->find(key_type{_token.data(), *g_allocator}); iter != g_map->end()) {
            return iter->second.data_id == _data_id &&
                   iter->second.replica_number == _replica_number;
        }

        return false;
    } // contains

    auto erase_pid(replica_token_view_type _token, pid_t _pid) -> std::optional<restorable_entry>
    {
        // Acquiring the upgradable lock allows more readers to use the replica access table.
        // This is required so that if the target entry to remove is found, the lock can be
        // transistioned to an exclusive lock atomically.
        bi::upgradable_lock lk{*g_mutex};

        if (const auto iter = g_map->find(key_type{_token.data(), *g_allocator}); iter != g_map->end()) {
            auto& pids = iter->second.agent_pids;

            if (const auto pos = std::find(pids.begin(), pids.end(), _pid); pos != pids.end()) {
                // We found the PID, acquire the exclusive lock atomically and remove the PID.
                bi::scoped_lock<bi::named_upgradable_mutex> write_lk{std::move(lk)};

                restorable_entry entry{_token, iter->second.data_id, iter->second.replica_number, *pos};

                pids.erase(pos);

                if (pids.empty()) {
                    g_map->erase(iter);
                }

                return entry;
            }
        }

        return std::nullopt;
    } // erase_pid

    auto erase_pid(pid_t _pid) -> void
    {
        bi::scoped_lock lk{*g_mutex};

        std::vector<const key_type*> keys_to_remove;

        for (auto&& [k, v] : *g_map) {
            auto end = std::end(v.agent_pids);
            v.agent_pids.erase(std::remove(std::begin(v.agent_pids), end, _pid), end);

            // Remember which entries have an empty PID list.
            if (v.agent_pids.empty()) {
                keys_to_remove.push_back(&k);
            }
        }

        for (auto&& k : keys_to_remove) {
            g_map->erase(*k);
        }
    } // erase_pid

    auto restore(const restorable_entry& _entry) -> void
    {
        bi::scoped_lock lk{*g_mutex};

        const key_type key{_entry.token.data(), *g_allocator};

        if (auto iter = g_map->find(key); iter != g_map->end()) {
            auto& [k, v] = *iter;

            if (v.data_id != _entry.data_id || v.replica_number != _entry.replica_number) {
                throw replica_access_table_error{"replica_access_table: Invalid data id or replica number"};
            }

            v.agent_pids.push_back(_entry.pid);
        }
        else {
            mapped_type value{_entry.data_id,
                              _entry.replica_number,
                              access_entry::container_type{{_entry.pid}, *g_allocator}};

            g_map->insert(value_type{key, value});
        }
    } // restore
} // namespace irods::experimental::replica_access_table

