#ifndef IRODS_SERVER_CONTROL_PLANE_HPP
#define IRODS_SERVER_CONTROL_PLANE_HPP

/// \file

#include "irods_lookup_table.hpp"
#include "server_control_plane_command.hpp"

#include "boost/thread.hpp"
#include "boost/atomic.hpp"
#include "boost/unordered_map.hpp"
#include "zmq.hpp"

namespace irods
{
    inline const std::string SERVER_CONTROL_OPTION_KW( "server_control_option" );
    inline const std::string SERVER_CONTROL_HOST_KW( "server_control_host" );
    inline const std::string SERVER_CONTROL_FORCE_AFTER_KW( "server_control_force_after" );
    inline const std::string SERVER_CONTROL_WAIT_FOREVER_KW( "server_control_wait_forever" );

    inline const std::string SERVER_CONTROL_SHUTDOWN( "server_control_shutdown" );
    inline const std::string SERVER_CONTROL_PAUSE( "server_control_pause" );
    inline const std::string SERVER_CONTROL_RESUME( "server_control_resume" );
    inline const std::string SERVER_CONTROL_STATUS( "server_control_status" );
    inline const std::string SERVER_CONTROL_PING( "server_control_ping" );

    inline const std::string SERVER_CONTROL_ALL_OPT( "all" );
    inline const std::string SERVER_CONTROL_HOSTS_OPT( "hosts" );
    inline const std::string SERVER_CONTROL_SUCCESS( "server_control_success" );

    inline const std::string SERVER_PAUSED_ERROR( "The server is Paused, resume before issuing any other commands" );

    // this is a hand-chosen polling time for the control plane
    static const size_t SERVER_CONTROL_POLLING_TIME_MILLI_SEC = 500;

    // derived from above - used to wait for the server to shut down or resume
    static const size_t SERVER_CONTROL_FWD_SLEEP_TIME_MILLI_SEC = SERVER_CONTROL_POLLING_TIME_MILLI_SEC / 4.0;

    class server_control_executor
    {
    public:
        server_control_executor(const std::string&); // port property

        server_control_executor(const server_control_executor&) = delete;
        server_control_executor& operator=(const server_control_executor&) = delete;

        // @brief callable operator for thread work
        void operator()();

    private:
        using ctrl_func_t = boost::function<error(const std::string&, const size_t, std::string&)>;
        using host_list_t = std::vector<std::string>;

        error process_operation(
            const zmq::message_t&, // incoming msg
            std::string& );        // outgoing text

        error extract_command_parameters(
            const irods::control_plane_command&, // incoming command
            std::string&,                        // command name
            std::string&,                        // command option
            std::string&,                        // wait option
            size_t&,                             // wait time in seconds
            host_list_t& );                      // hostnames

        error process_host_list(
            const std::string&, // command
            const std::string&, // wait option
            const size_t&,      // wait time in seconds
            const host_list_t&, // hostnames
            std::string& );     // outgoing text

        error validate_host_list(
            const host_list_t&, // irods hostnames
            const host_list_t&, // incoming hostnames
            host_list_t& );     // valid hostnames

        error get_resource_host_names(
            host_list_t& );     // fetched hostnames

        error notify_provider_and_local_servers_preop(
            const std::string&, // command
            const std::string&, // command option
            const std::string&, // wait option
            const size_t&,      // wait seconds
            const host_list_t&, // irods hostnames
            std::string& );     // output

        error notify_provider_and_local_servers_postop(
            const std::string&, // command
            const std::string&, // command option
            const std::string&, // wait option
            const size_t&,      // wait seconds
            const host_list_t&, // irods hostnames
            std::string& );     // output

        error forward_command(
            const std::string&, // command name
            const std::string&, // host
            const std::string&, // port keyword
            const std::string&, // wait option
            const size_t&,      // wait seconds
            std::string& );     // output

        bool is_host_in_list(
            const std::string&,   // host name in question
            const host_list_t& ); // list of candidates

        const std::string port_prop_;
        boost::unordered_map<std::string, ctrl_func_t> op_map_;
        std::string local_server_hostname_;
        std::string provider_hostname_;
    }; // class server_control_executor

    class server_control_plane
    {
    public:
        // @brief default constructor taking the port property
        server_control_plane(const std::string&);

        server_control_plane(const server_control_plane&) = delete;
        server_control_plane& operator=(const server_control_plane&) = delete;

        // @brief destructor
        ~server_control_plane();

    private:
        // @brief functor which manages the control
        server_control_executor control_executor_;

        // @brief thread which manages the control loop
        boost::thread control_thread_;
    }; // server_control_plane
} // namespace irods

#endif // IRODS_SERVER_CONTROL_PLANE_HPP

