#include "irods/rodsAgent.hpp"

#include "irods/icatHighLevelRoutines.hpp"
#include "irods/initServer.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_auth_constants.hpp"
#include "irods/irods_auth_factory.hpp"
#include "irods/irods_auth_manager.hpp"
#include "irods/irods_auth_object.hpp"
#include "irods/irods_auth_plugin.hpp"
#include "irods/irods_client_api_table.hpp"
#include "irods/irods_client_server_negotiation.hpp"
#include "irods/irods_dynamic_cast.hpp"
#include "irods/irods_environment_properties.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_network_factory.hpp"
#include "irods/irods_pack_table.hpp"
#include "irods/irods_re_plugin.hpp"
#include "irods/irods_re_serialization.hpp"
#include "irods/irods_server_api_table.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/irods_server_state.hpp"
#include "irods/irods_signal.hpp"
#include "irods/irods_socket_information.hpp"
#include "irods/irods_threads.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/plugin_lifetime_manager.hpp"
#include "irods/procLog.h"
#include "irods/reconstants.hpp"
#include "irods/replica_access_table.hpp"
#include "irods/replica_state_table.hpp"
#include "irods/rsApiHandler.hpp"
#include "irods/server_utilities.hpp"
#include "irods/sockCommNetworkInterface.hpp"
#include "irods/sslSockComm.h"
#include "irods/version.hpp"

#include <csignal>
#include <cstdlib>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <fmt/format.h>

#include <cstring>
#include <memory>
#include <array>
#include <string>
#include <string_view>

#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
#  include <sanitizer/lsan_interface.h>
#endif

namespace log = irods::experimental::log;

// clang-format off
using log_agent_factory = irods::experimental::log::agent_factory;
using log_agent         = irods::experimental::log::agent;
// clang-format on

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
ssize_t receiveSocketFromSocket(int readFd, int* socket)
{
    struct msghdr msg; // NOLINT(cppcoreguidelines-pro-type-member-init)
    std::memset(&msg, 0, sizeof(struct msghdr));

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    std::array<char, 1024> message_buf{};
    struct iovec io = {.iov_base = message_buf.data(), .iov_len = message_buf.size()};
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    std::array<char, 1024> control_buf{};
    msg.msg_control = control_buf.data();
    msg.msg_controllen = control_buf.size();

    ssize_t n = recvmsg(readFd, &msg, MSG_WAITALL);
    if (n <= 0) {
        return n;
    }

    cmsghdr* cmptr = CMSG_FIRSTHDR(&msg); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
    unsigned char* data = CMSG_DATA(cmptr); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    int theSocket = *((int*) data); // NOLINT(google-readability-casting, cppcoreguidelines-pro-type-cstyle-cast)
    *socket = theSocket;

    return n;
} // receiveSocketFromSocket

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
int receiveDataFromServer(int conn_tmp_socket)
{
    ssize_t num_bytes{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init, cppcoreguidelines-avoid-magic-numbers)
    std::array<char, 1024> in_buf{}; // NOLINT(readability-magic-numbers)
    bool data_complete = false;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init, cppcoreguidelines-avoid-magic-numbers)
    std::array<char, 256> ack_buffer; // NOLINT(readability-magic-numbers)
    std::snprintf(ack_buffer.data(), ack_buffer.size(), "OK"); // NOLINT(cppcoreguidelines-pro-type-vararg)

    while (!data_complete) {
        std::memset(in_buf.data(), 0, in_buf.size());
        num_bytes = recv(conn_tmp_socket, in_buf.data(), in_buf.size(), 0);

        if (num_bytes < 0) {
            // NOLINTNEXTLINE(concurrency-mt-unsafe)
            log_agent_factory::error("Error receiving data from rodsServer, errno = [{}][{}]", errno, strerror(errno));
            return SYS_SOCK_READ_ERR;
        }

        if (num_bytes == 0) {
            log_agent_factory::error("Received 0 bytes from rodsServer");
            return SYS_SOCK_READ_ERR;
        }

        char* tokenized_strings = std::strtok(in_buf.data(), ";"); // NOLINT(concurrency-mt-unsafe)

        while (tokenized_strings != nullptr) {
            const std::string_view tmpStr = tokenized_strings;

            if (tmpStr == "end_of_vars") {
                data_complete = true;

                // Send acknowledgement that all data has been received
                num_bytes = send(conn_tmp_socket, ack_buffer.data(), std::strlen(ack_buffer.data()) + 1, 0);
                if (num_bytes < 0) {
                    constexpr const char* msg = "Error sending acknowledgment to rodsServer, errno = [{}][{}]";
                    // NOLINTNEXTLINE(concurrency-mt-unsafe)
                    log_agent_factory::error(msg, errno, strerror(errno));
                    return SYS_SOCK_READ_ERR;
                }

                break;
            }

            const auto pos = tmpStr.find('=');

            if (pos == std::string_view::npos) {
                // No equal sign was found.
                continue;
            }

            const auto lhs = tmpStr.substr(0, pos);
            const auto rhs = tmpStr.substr(pos + 1, tmpStr.size());

            // NOLINTNEXTLINE(concurrency-mt-unsafe)
            if (setenv(std::string{lhs}.c_str(), std::string{rhs}.c_str(), 1) < 0) {
                constexpr const char* msg_fmt = "setenv([{}], [{}], 1) failed with errno = [{}][{}]";
                // NOLINTNEXTLINE(concurrency-mt-unsafe)
                const auto msg = fmt::format(msg_fmt, lhs, rhs, errno, strerror(errno));
                log_agent_factory::error(ERROR(SYS_INTERNAL_ERR, msg).result());
            }

            // NOLINTNEXTLINE(concurrency-mt-unsafe)
            tokenized_strings = std::strtok(nullptr, ";");
        }
    }

    int newSocket{};
    num_bytes = receiveSocketFromSocket(conn_tmp_socket, &newSocket);
    if (num_bytes < 0) {
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        log_agent_factory::error("Error receiving socket from rodsServer, errno = [{}]", errno, strerror(errno));
        return SYS_SOCK_READ_ERR;
    }

    if (num_bytes == 0) {
        log_agent_factory::error("Received 0 bytes from rodsServer");
        return SYS_SOCK_READ_ERR;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init, cppcoreguidelines-avoid-magic-numbers)
    std::array<char, 16> socket_buf; // NOLINT(readability-magic-numbers)
    std::snprintf(socket_buf.data(), socket_buf.size(), "%d", newSocket); // NOLINT(cppcoreguidelines-pro-type-vararg)

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    unsigned int len = std::snprintf(ack_buffer.data(), ack_buffer.size(), "%d", getpid());
    num_bytes = send(conn_tmp_socket, ack_buffer.data(), len, 0);
    if (num_bytes < 0) {
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        log_agent_factory::error("Error sending agent pid to rodsServer, errno = [{}]", errno, strerror(errno));
        return SYS_SOCK_READ_ERR;
    }

    if (setenv(SP_NEW_SOCK, socket_buf.data(), 1) < 0) { // NOLINT(concurrency-mt-unsafe)
        constexpr const char* msg_fmt = "setenv([{}], [{}], 1) failed with errno = [{}][{}]";
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        const auto msg = fmt::format(msg_fmt, SP_NEW_SOCK, socket_buf.data(), errno, strerror(errno));
        log_agent_factory::error(ERROR(SYS_INTERNAL_ERR, msg).result());
    }

    const auto status = close(conn_tmp_socket);
    if (status < 0) {
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        log_agent_factory::error("close(conn_tmp_socket) failed with errno = [{}]: {}", errno, strerror(errno));
    }

    return status;
} // receiveDataFromServer

void cleanup()
{
    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if (!ret.ok()) {
        log_agent::error(PASS(ret).result());
    }

    if (INITIAL_DONE == InitialState) {
        close_all_l1_descriptors(*ThisComm);

        // This agent's PID must be erased from all replica access table entries as it will soon no longer exist.
        irods::experimental::replica_access_table::erase_pid(getpid());

        irods::replica_state_table::deinit();

        disconnectAllSvrToSvrConn();
    }

    if (irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role) {
        disconnectRcat();
    }

    irods::re_plugin_globals->global_re_mgr.call_stop_operations();
} // cleanup

void cleanupAndExit(int status)
{
    cleanup();

    if (status >= 0) {
        std::exit(0); // NOLINT(concurrency-mt-unsafe)
    }

    std::exit(1); // NOLINT(concurrency-mt-unsafe)
} // cleanupAndExit

void irodsAgentSignalExit([[maybe_unused]] int _signal)
{
    int agent_pid{};
    int agent_status{};

    while ((agent_pid = waitpid(-1, &agent_status, WNOHANG)) > 0) {
        rmProcLog(agent_pid);
    }

#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
    // Calling this function is likely not async-signal-safe, but that's okay because
    // the code has been compiled with Address Sanitizer enabled. For that reason, we
    // can assume that the binary is not running in a production environment.
    __lsan_do_leak_check();
#endif

    _exit(_signal);
}

void reap_terminated_agents()
{
    int agent_pid{};
    int agent_status{};

    while ((agent_pid = waitpid(-1, &agent_status, WNOHANG)) > 0) {
        log_agent_factory::trace("Reaped agent [{}] ...", agent_pid);

        if (WIFEXITED(agent_status)) { // NOLINT(hicpp-signed-bitwise)
            const int exit_status = WEXITSTATUS(agent_status); // NOLINT(hicpp-signed-bitwise)
            if (exit_status != 0) {
                log_agent_factory::error("Agent process [{}] exited with status [{}].", agent_pid, exit_status);
            }
            else {
                log_agent_factory::debug("Agent process [{}] exited with status [{}].", agent_pid, exit_status);
            }
        }
        else if (WIFSIGNALED(agent_status)) { // NOLINT(hicpp-signed-bitwise)
            constexpr const char* msg = "Agent process [{}] terminated by signal [{}].";
            // NOLINTNEXTLINE(hicpp-signed-bitwise)
            log_agent_factory::error(msg, agent_pid, WTERMSIG(agent_status));
        }
        else {
            log_agent_factory::error(
                "Agent process [{}] terminated with unusual status [{}].", agent_pid, agent_status);
        }

        rmProcLog(agent_pid);

        log_agent_factory::trace("Removing agent PID [{}] from replica access table ...", agent_pid);
        irods::experimental::replica_access_table::erase_pid(agent_pid);
    }
} // reap_terminated_agents

void set_eviction_age_for_dns_and_hostname_caches()
{
    using key_path_t = irods::configuration_parser::key_path_t;

    // Update the eviction age for DNS cache entries.
    irods::set_server_property(
        key_path_t{irods::KW_CFG_ADVANCED_SETTINGS, irods::KW_CFG_DNS_CACHE, irods::KW_CFG_EVICTION_AGE_IN_SECONDS},
        irods::get_dns_cache_eviction_age());

    // Update the eviction age for hostname cache entries.
    irods::set_server_property(
        key_path_t{
            irods::KW_CFG_ADVANCED_SETTINGS, irods::KW_CFG_HOSTNAME_CACHE, irods::KW_CFG_EVICTION_AGE_IN_SECONDS},
        irods::get_hostname_cache_eviction_age());
} // set_eviction_age_for_dns_and_hostname_caches

void set_log_levels_for_all_log_categories()
{
    log::agent::set_level(log::get_level_from_config(irods::KW_CFG_LOG_LEVEL_CATEGORY_AGENT));
    log::legacy::set_level(log::get_level_from_config(irods::KW_CFG_LOG_LEVEL_CATEGORY_LEGACY));
    log::resource::set_level(log::get_level_from_config(irods::KW_CFG_LOG_LEVEL_CATEGORY_RESOURCE));
    log::database::set_level(log::get_level_from_config(irods::KW_CFG_LOG_LEVEL_CATEGORY_DATABASE));
    log::authentication::set_level(log::get_level_from_config(irods::KW_CFG_LOG_LEVEL_CATEGORY_AUTHENTICATION));
    log::api::set_level(log::get_level_from_config(irods::KW_CFG_LOG_LEVEL_CATEGORY_API));
    log::microservice::set_level(log::get_level_from_config(irods::KW_CFG_LOG_LEVEL_CATEGORY_MICROSERVICE));
    log::network::set_level(log::get_level_from_config(irods::KW_CFG_LOG_LEVEL_CATEGORY_NETWORK));
    log::rule_engine::set_level(log::get_level_from_config(irods::KW_CFG_LOG_LEVEL_CATEGORY_RULE_ENGINE));
    log::sql::set_level(log::get_level_from_config(irods::KW_CFG_LOG_LEVEL_CATEGORY_SQL));
} // set_log_levels_for_all_log_categories

void setup_signal_handlers()
{
    signal(SIGINT, irodsAgentSignalExit);
    signal(SIGHUP, irodsAgentSignalExit);
    signal(SIGTERM, irodsAgentSignalExit);
    signal(SIGCHLD, SIG_DFL); // Setting SIGCHLD to SIG_IGN is not portable.
    signal(SIGUSR1, irodsAgentSignalExit);
    signal(SIGPIPE, SIG_IGN);

    irods::set_unrecoverable_signal_handlers();
} // setup_signal_handlers

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
int setup_unix_domain_socket_for_listening(sockaddr_un _socket_addr)
{
    const auto sfd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sfd < 0) {
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        log_agent_factory::error("Unable to create socket in runIrodsAgent, errno = [{}]: {}", errno, strerror(errno));
        return SYS_SOCK_OPEN_ERR;
    }

    // Delete socket if it already exists.
    unlink(_socket_addr.sun_path); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    if (bind(sfd, reinterpret_cast<struct sockaddr*>(&_socket_addr), sizeof(sockaddr_un)) < 0) {
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        log_agent_factory::error("Unable to bind socket in runIrodsAgent, errno [{}]: {}", errno, strerror(errno));
        return SYS_SOCK_BIND_ERR;
    }

    if (listen(sfd, 5) < 0) { // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        constexpr const char* msg = "Unable to set up socket for listening in runIrodsAgent, errno [{}]: {}";
        log_agent_factory::error(msg, errno, strerror(errno)); // NOLINT(concurrency-mt-unsafe)
        return SYS_SOCK_LISTEN_ERR;
    }

    return sfd;
} // setup_unix_domain_socket_for_listening

int runIrodsAgentFactory(sockaddr_un agent_addr)
{
    namespace log = irods::experimental::log;

    log::set_server_type("agent_factory");

    irods::server_properties::instance().capture();
    log_agent_factory::set_level(log::get_level_from_config(irods::KW_CFG_LOG_LEVEL_CATEGORY_AGENT_FACTORY));

    log_agent_factory::info("Initializing agent factory ...");

    setup_signal_handlers();

    initProcLog();

    const auto listen_socket = setup_unix_domain_socket_for_listening(agent_addr);
    if (listen_socket < 0) {
        return listen_socket;
    }

    struct sockaddr_un client_addr;

    const auto client_socket = [listen_socket, &client_addr] {
        unsigned int len = sizeof(sockaddr_un);
        return accept(listen_socket, reinterpret_cast<struct sockaddr*>(&client_addr), &len);
    }();

    if (client_socket < 0) {
        constexpr const char* msg = "Failed to accept client socket in runIrodsAgent, errno [{}]: {}";
        log_agent_factory::error(msg, errno, strerror(errno)); // NOLINT(concurrency-mt-unsafe)
        return SYS_SOCK_ACCEPT_ERR;
    }

    int conn_tmp_socket;

    while (true) {
        reap_terminated_agents();

        fd_set read_socket;
        FD_ZERO(&read_socket);
        FD_SET(client_socket, &read_socket);

        struct timeval time_out;
        time_out.tv_sec = 0;
        time_out.tv_usec = 30 * 1000;
        const int ready = select(client_socket + 1, &read_socket, nullptr, nullptr, &time_out);

        // Check the ready socket
        if (-1 == ready) {
            if (EINTR == errno) {
                // Caught a signal, return to the select() call
                log_agent_factory::debug("select() was interrupted in the agent factory process, continuing ...");
                continue;
            }

            // select() failed, quit
            log_agent_factory::error("select() failed with errno = [{}]: {}", errno, strerror(errno));
            return SYS_SOCK_SELECT_ERR;
        }

        if (ready == 0) {
            continue;
        }

        // select returned, attempt to receive data
        // If 0 bytes are received, socket has been closed
        // If a socket address is on the line, create it and fork a child process
        char in_buf[1024]{};
        const ssize_t bytes_received = recv(client_socket, in_buf, sizeof(in_buf), 0);

        if (-1 == bytes_received) {
            log_agent_factory::error("Error receiving data from rodsServer, errno = [{}]: {}", errno, strerror(errno));
            return SYS_SOCK_READ_ERR;
        }

        if (0 == bytes_received) {
            log_agent_factory::info("The rodsServer socket peer has shut down.");
            return 0;
        }

        // Assume that we have received valid data over the socket connection.
        // Set up the temporary (per-agent) sockets.
        sockaddr_un tmp_socket_addr{};
        tmp_socket_addr.sun_family = AF_UNIX;
        std::strncpy(tmp_socket_addr.sun_path, in_buf, sizeof(tmp_socket_addr.sun_path));
        unsigned int len = sizeof(tmp_socket_addr);

        const auto listen_tmp_socket = socket(AF_UNIX, SOCK_STREAM, 0);

        // Delete socket if it already exists.
        unlink(tmp_socket_addr.sun_path);

        if (bind(listen_tmp_socket, (struct sockaddr*) &tmp_socket_addr, len) == -1) {
            constexpr auto ec = SYS_SOCK_BIND_ERR;
            log_agent_factory::error(ERROR(ec, "Unable to bind socket in receiveDataFromServer").result());
            return ec;
        }

        if (listen(listen_tmp_socket, 5) == -1) {
            constexpr auto ec = SYS_SOCK_LISTEN_ERR;
            constexpr const char* msg = "Failed to set up socket for listening in receiveDataFromServer";
            log_agent_factory::error(ERROR(ec, msg).result());
            return ec;
        }

        // Send acknowledgement that socket has been created.
        constexpr const char ack_buffer[] = "OK";
        const auto bytes_sent = send(client_socket, ack_buffer, sizeof(ack_buffer), 0);
        if (bytes_sent < 0) {
            constexpr const char* msg = "[{}] - Error sending acknowledgment to rodsServer, errno = [{}][{}]";
            log_agent_factory::error(msg, __func__, errno, strerror(errno));
            return SYS_SOCK_READ_ERR;
        }

        // Wait for connection message from main server.
        std::memset(in_buf, 0, sizeof(in_buf));
        recv(client_socket, in_buf, sizeof(in_buf), 0);
        if (std::strncmp(in_buf, "connection_successful", sizeof(in_buf)) != 0) {
            constexpr const char* msg = "[{}:{}] - received failure message in connecting to socket from server";
            log_agent_factory::error(msg, __func__, __LINE__);

            if (close(listen_tmp_socket) < 0) {
                log_agent_factory::error(
                    "close(listen_tmp_socket) failed with errno = [{}]: {}", errno, strerror(errno));
            }

            continue;
        }

        conn_tmp_socket = accept(listen_tmp_socket, (struct sockaddr*) &tmp_socket_addr, &len);
        if (-1 == conn_tmp_socket) {
            constexpr auto ec = SYS_SOCK_ACCEPT_ERR;
            log_agent_factory::error(ERROR(ec, "Failed to accept client socket in receiveDataFromServer").result());
            return ec;
        }

        //
        // Data is ready on conn_socket, fork a child process to handle it.
        //

        log_agent_factory::trace("Spawning agent to handle request ...");

        const auto agent_pid = fork();

        // This socket will not be used by the agent factory or agent, so close it.
        close(listen_tmp_socket);

        if (agent_pid == 0) {
            // This is the child process.
            // Agent logic starts outside of the while-loop.
            break;
        }
        else if (agent_pid > 0) {
            // This is the parent process.
            // Clean up and prepare to fork more agents upon request.
            close(conn_tmp_socket);
        }
        else if (agent_pid < 0) {
            log_agent_factory::critical("Agent factory failed to fork agent. Shutting down agent factory ...");

            close(client_socket);
            close(listen_socket);

            return SYS_FORK_ERROR;
        }
    } // Agent factory main loop.

    //
    // This is where the agent logic actually begins.
    //

    int status{};

    try {
        log::set_server_type("agent");

        // Reload irods_environment.json and server_config.json for the newly forked agent process.
        irods::environment_properties::instance().capture();
        irods::server_properties::instance().capture();

        log_agent::trace("Agent forked. Initializing ...");

        close(listen_socket);

        // Restore signal dispositions for agents.
        std::signal(SIGABRT, SIG_DFL);
        std::signal(SIGINT, SIG_DFL);
        std::signal(SIGHUP, SIG_DFL);
        std::signal(SIGTERM, SIG_DFL);
        std::signal(SIGCHLD, SIG_DFL);
        std::signal(SIGUSR1, SIG_DFL);
        std::signal(SIGPIPE, SIG_DFL);

        status = receiveDataFromServer(conn_tmp_socket);
        if (status < 0) {
            log_agent::error("receiveDataFromServer failed [error_code=[{}]].", status);
        }

        close(conn_tmp_socket);

        set_eviction_age_for_dns_and_hostname_caches();
        set_log_levels_for_all_log_categories();

        if (const auto err = setRECacheSaltFromEnv(); !err.ok()) {
            log_agent::error("rodsAgent::main: Failed to set RE cache mutex name\n%s", err.result());
            return SYS_INTERNAL_ERR;
        }
    }
    catch (const irods::exception& e) {
        log_agent::error("Agent initialization error: {}", e.what());
        return e.code() == -1 ? SYS_UNKNOWN_ERROR : e.code();
    }
    catch (const std::exception& e) {
        log_agent::error("Agent initialization error: {}", e.what());
        return SYS_LIBRARY_ERROR;
    }

    RsComm rsComm{};

    log::set_error_object(&rsComm.rError);
    irods::at_scope_exit release_error_stack{[] { log::set_error_object(nullptr); }};

    //std::memset(&rsComm, 0, sizeof(RsComm));
    rsComm.thread_ctx = static_cast<thread_context*>(std::malloc(sizeof(thread_context)));

    status = initRsCommWithStartupPack(&rsComm, nullptr);

    // manufacture a network object for comms
    irods::network_object_ptr net_obj;
    irods::error ret = irods::network_factory(&rsComm, net_obj);
    if (!ret.ok()) {
        log_agent::error(PASS(ret).result());
    }

    if (status < 0) {
        log_agent::error("initRsCommWithStartupPack error: [{}]", status);
        sendVersion(net_obj, status, 0, nullptr, 0);
        cleanupAndExit(status);
    }

    irods::re_plugin_globals.reset(new irods::global_re_plugin_mgr);
    irods::re_plugin_globals->global_re_mgr.call_start_operations();

    status = getRodsEnv(&rsComm.myEnv);

    if (status < 0) {
        log_agent::error("agentMain :: getRodsEnv failed");
        sendVersion(net_obj, SYS_AGENT_INIT_ERR, 0, nullptr, 0);
        cleanupAndExit(status);
    }

    // load server side pluggable api entries
    irods::api_entry_table&  RsApiTable   = irods::get_server_api_table();
    irods::pack_entry_table& ApiPackTable = irods::get_pack_table();
    ret = irods::init_api_table(RsApiTable, ApiPackTable, false);
    if (!ret.ok()) {
        log_agent::error(PASS(ret).result());
        return 1;
    }

    // load client side pluggable api entries
    irods::api_entry_table& RcApiTable = irods::get_client_api_table();
    ret = irods::init_api_table(RcApiTable, ApiPackTable, false);
    if (!ret.ok()) {
        log_agent::error(PASS(ret).result());
        return 1;
    }

    std::string svc_role;
    ret = get_catalog_service_role(svc_role);
    if (!ret.ok()) {
        log_agent::error(PASS(ret).result());
        return ret.code();
    }

    if (irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role) {
        if (std::strstr(rsComm.myEnv.rodsDebug, "CAT") != nullptr) {
            chlDebug(rsComm.myEnv.rodsDebug);
        }
    }

    status = initAgent(RULE_ENGINE_TRY_CACHE, &rsComm);

    if (status < 0) {
        log_agent::error("agentMain :: initAgent failed: {}", status);
        sendVersion(net_obj, SYS_AGENT_INIT_ERR, 0, nullptr, 0);
        cleanupAndExit(status);
    }

    if (rsComm.clientUser.userName[0] != '\0') {
        status = chkAllowedUser(rsComm.clientUser.userName, rsComm.clientUser.rodsZone);

        if (status < 0) {
            sendVersion(net_obj, status, 0, nullptr, 0);
            cleanupAndExit(status);
        }
    }

    // handle negotiations with the client regarding TLS if requested
    // this scope block makes valgrind happy
    {
        std::string neg_results;
        ret = irods::client_server_negotiation_for_server(net_obj, neg_results);
        if (!ret.ok() || neg_results == irods::CS_NEG_FAILURE) {
            // send a 'we failed to negotiate' message here??
            // or use the error stack rule engine thingie
            log_agent::error(PASS(ret).result());
            sendVersion(net_obj, SERVER_NEGOTIATION_ERROR, 0, nullptr, 0);
            cleanupAndExit(ret.code());
        }
        else {
            // copy negotiation results to comm for action by network objects
            std::snprintf(rsComm.negotiation_results, sizeof(rsComm.negotiation_results), "%s", neg_results.c_str());
        }
    }

    // send the server version and status as part of the protocol. Put
    // rsComm.reconnPort as the status
    ret = sendVersion(net_obj, status, rsComm.reconnPort, rsComm.reconnAddr, rsComm.cookie);

    if (!ret.ok()) {
        log_agent::error(PASS(ret).result());
        sendVersion(net_obj, SYS_AGENT_INIT_ERR, 0, nullptr, 0);
        cleanupAndExit(status);
    }

    logAgentProc(&rsComm);

    // call initialization for network plugin as negotiated
    irods::network_object_ptr new_net_obj;
    ret = irods::network_factory(&rsComm, new_net_obj);
    if (!ret.ok()) {
        return ret.code();
    }

    ret = sockAgentStart(new_net_obj);
    if (!ret.ok()) {
        log_agent::error(PASS(ret).result());
        return ret.code();
    }

    const auto cleanup_and_free_rsComm_members = [&rsComm] {
        cleanup();
        if (rsComm.thread_ctx) {
            std::free(rsComm.thread_ctx); // NOLINT(cppcoreguidelines-no-malloc, cppcoreguidelines-owning-memory)
        }
        if (rsComm.auth_scheme) {
            std::free(rsComm.auth_scheme); // NOLINT(cppcoreguidelines-no-malloc, cppcoreguidelines-owning-memory)
        }
    };

    new_net_obj->to_server(&rsComm);
    status = agentMain(&rsComm);

    // call initialization for network plugin as negotiated
    ret = sockAgentStop( new_net_obj );
    if (!ret.ok()) {
        log_agent::error(PASS(ret).result());
        cleanup_and_free_rsComm_members();
        return ret.code();
    }

    new_net_obj->to_server(&rsComm);

    cleanup_and_free_rsComm_members();

    // clang-format off
    (0 == status)
        ? log_agent::debug("Agent [{}] exiting with status = {}", getpid(), status)
        : log_agent::error("Agent [{}] exiting with status = {}", getpid(), status);
    // clang-format on

#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
    // This function must be called here due to the use of _exit() (just below). Address Sanitizer (ASan)
    // relies on std::atexit handlers to report its findings. _exit() does not trigger any of the handlers
    // registered by ASan, therefore, we manually run ASan just before the agent exits.
    __lsan_do_leak_check();
#endif

    // _exit() must be called here due to a design limitation involving forked processes and mutexes.
    //
    // It has been observed that if the agent factory is respawned by the main server process, global
    // mutexes will be locked 99% of the time. These global mutexes can never be unlocked following the
    // call to fork().
    //
    // iRODS makes use of C++ libraries that make assertions around the handling of mutexes (e.g. boost::mutex).
    // If these assertions are violated, SIGABRT is triggered. For that reason, we cannot allow agents to
    // execute std::exit() or return up the call chain. Doing so would result in SIGABRT. For the most part,
    // using _exit() is perfectly fine here because this is the final step in shutting down the agent process.
    //
    // The key word here is process. Following this call, the OS will reclaim all memory associated with
    // the terminated agent process.
    _exit((0 == status) ? 0 : 1);
}

static void set_rule_engine_globals(RsComm* _comm)
{
    irods::set_server_property<std::string>(irods::CLIENT_USER_NAME_KW, _comm->clientUser.userName);
    irods::set_server_property<std::string>(irods::CLIENT_USER_ZONE_KW, _comm->clientUser.rodsZone);
    irods::set_server_property<int>(irods::CLIENT_USER_PRIV_KW, _comm->clientUser.authInfo.authFlag);
    irods::set_server_property<std::string>(irods::PROXY_USER_NAME_KW, _comm->proxyUser.userName);
    irods::set_server_property<std::string>(irods::PROXY_USER_ZONE_KW, _comm->proxyUser.rodsZone);
    irods::set_server_property<int>(irods::PROXY_USER_PRIV_KW, _comm->clientUser.authInfo.authFlag);
} // set_rule_engine_globals

int agentMain(RsComm* rsComm)
{
    if (!rsComm) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    int status = 0;

    // compiler backwards compatibility hack
    // see header file for more details
    irods::dynamic_cast_hack();

    while (status >= 0) {
        // set default to the native auth scheme here.
        if (!rsComm->auth_scheme) {
            rsComm->auth_scheme = strdup("native");
        }
        // The following is an artifact of the legacy authentication plugins. This operation is
        // only useful for certain plugins which are not supported in 4.3.0, so it is being
        // left out of compilation for now. Once we have determined that this is safe to do in
        // general, this section can be removed.
#if 0
        // construct an auth object based on the scheme specified in the comm
        irods::auth_object_ptr auth_obj;
        if (const auto err = irods::auth_factory(rsComm->auth_scheme, &rsComm->rError, auth_obj); !err.ok()) {
            irods::experimental::api::plugin_lifetime_manager::destroy();

            irods::log(PASSMSG(fmt::format(
                "Failed to factory an auth object for scheme: \"{}\".",
                rsComm->auth_scheme), err));

            return err.code();
        }

        irods::plugin_ptr ptr;
        if (const auto err = auth_obj->resolve(irods::AUTH_INTERFACE, ptr); !err.ok()) {
            irods::experimental::api::plugin_lifetime_manager::destroy();

            irods::log(PASSMSG(fmt::format(
                "Failed to resolve the auth plugin for scheme: \"{}\".",
                rsComm->auth_scheme), err));

            return err.code();
        }

        irods::auth_ptr auth_plugin = boost::dynamic_pointer_cast<irods::auth>(ptr);

        // Call agent start
        if (const auto err = auth_plugin->call<const char*>(rsComm, irods::AUTH_AGENT_START, auth_obj, ""); !err.ok()) {
            irods::experimental::api::plugin_lifetime_manager::destroy();

            irods::log(PASSMSG(fmt::format(
                "Failed during auth plugin agent start for scheme: \"{}\".",
                rsComm->auth_scheme), err));

            return err.code();
        }
#endif

        // add the user info to the server properties for
        // reach by the operation wrapper for access by the
        // dynamic policy enforcement points
        try {
            set_rule_engine_globals(rsComm);
        }
        catch (const irods::exception& e) {
            log_agent::error("set_rule_engine_globals failed:\n{}", e.what());
        }

        if (rsComm->ssl_do_accept) {
            status = sslAccept( rsComm );
            if (status < 0) {
                log_agent::error("sslAccept failed in agentMain with status {}", status);
            }
            rsComm->ssl_do_accept = 0;
        }
        if (rsComm->ssl_do_shutdown) {
            status = sslShutdown(rsComm);
            if (status < 0) {
                log_agent::error("sslShutdown failed in agentMain with status {}", status);
            }
            rsComm->ssl_do_shutdown = 0;
        }

        status = readAndProcClientMsg(rsComm, READ_HEADER_TIMEOUT);
        if (status < 0) {
            if (status == DISCONN_STATUS) {
                status = 0;
                break;
            }
        }
    }

    irods::experimental::api::plugin_lifetime_manager::destroy();

    // determine if we even need to connect, break the
    // infinite reconnect loop.
    if (!resc_mgr.need_maintenance_operations()) {
        return status;
    }

    // find the icat host
    rodsServerHost_t* rodsServerHost = 0;
    status = getRcatHost(PRIMARY_RCAT, 0, &rodsServerHost);
    if (status < 0) {
        log_agent::error(ERROR(status, "getRcatHost failed.").result());
        return status;
    }

    // connect to the icat host
    status = svrToSvrConnect( rsComm, rodsServerHost );
    if ( status < 0 ) {
        log_agent::error(ERROR(status, "svrToSvrConnect failed.").result());
        return status;
    }

    // call post disconnect maintenance operations before exit
    status = resc_mgr.call_maintenance_operations(rodsServerHost->conn);

    return status;
} // agentMain
