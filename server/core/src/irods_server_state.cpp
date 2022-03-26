#include "irods/irods_server_state.hpp"

#include "irods/rodsErrorTable.h"
#include "irods/shared_memory_object.hpp"

#include <fmt/format.h>

#include <unistd.h>
#include <sys/types.h>

#include <chrono>
#include <memory>
#include <string>

namespace
{
    namespace ipc = irods::experimental::interprocess;

    using ipc_object_type = ipc::shared_memory_object<irods::server_state::server_state>;

    //
    // Global Variables
    //

    // On initialization, holds the PID of the process that initialized the server state.
    // This ensures that only the process that initialized the system can deinitialize it.
    pid_t g_owner_pid = 0;

    // A pointer to the shared memory object that will hold the server's state.
    // Allocating on the heap allows us to know when the server state is constructed/destructed.
    std::unique_ptr<ipc_object_type> g_state;

    // The following variable defines the name of the shared memory file.
    std::string g_shared_memory_name;

    auto current_timestamp_in_seconds() noexcept -> std::int64_t
    {
        using std::chrono::system_clock;
        using std::chrono::seconds;
        using std::chrono::duration_cast;

        return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
    } // current_timestamp_in_seconds
} // anonymous namespace

namespace irods::server_state
{
    auto init() -> void 
    {
        // While this doesn't protect grandpa's shared memory from threads or child processes
        // launched via fork-exec, it does protect the shared memory from processes that only
        // fork (e.g. Agent Factory and Agents).
        if (getpid() == g_owner_pid) {
            return;
        }

        g_owner_pid = getpid();
        g_shared_memory_name = fmt::format("irods_server_state_{}_{}", getpid(), current_timestamp_in_seconds());

        // Due to the implementation of shared_memory_object, the object might not be
        // set to the value passed. Relying on the constructor to do this is incorrect
        // because shared_memory_object only initializes the object when the shared
        // memory is first created. If the shared memory exists, the values passed to
        // the shared_memory_object will be ignored.
        g_state.reset(new ipc_object_type{g_shared_memory_name, server_state::running});
    } // init

    auto deinit() -> void 
    {
        // Only allow the process that called init() to remove the shared memory.
        if (getpid() != g_owner_pid) {
            return;
        }

        try {
            if (g_state) {
                g_state->remove();
            }
        }
        catch (...) {}
    } // deinit

    auto get_state() -> server_state 
    {
        return g_state->atomic_exec_read([](server_state _value) {
            return _value;
        });
    } // get_state

    auto set_state(server_state _new_state) -> irods::error 
    {
        return g_state->atomic_exec([_new_state](server_state& _value) {
            _value = _new_state;
            return SUCCESS();
        });
    } // set_state

    auto to_string(server_state _state) -> std::string_view
    {
        switch (_state) {
            // clang-format off
            case server_state::running: return "server_state_running";
            case server_state::paused:  return "server_state_paused";
            case server_state::stopped: return "server_state_stopped";
            case server_state::exited:  return "server_state_exited";
            // clang-format on
        }

        throw std::invalid_argument{"Server state not supported"};
    } // to_string
} // namespace irods::server_state

