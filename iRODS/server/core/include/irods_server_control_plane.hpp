


#ifndef SERVER_CONTROL_PLANE_HPP
#define SERVER_CONTROL_PLANE_HPP

#include "zmq.hpp"
#include "boost/thread.hpp"
#include "boost/atomic.hpp"
#include "irods_lookup_table.hpp"
#include "server_control_plane_command.hpp"
#include "boost/unordered_map.hpp"

namespace irods {
    const std::string SERVER_CONTROL_OPTION_KW( "server_control_option" );
    const std::string SERVER_CONTROL_HOST_KW( "server_control_host" );
    const std::string SERVER_CONTROL_FORCE_AFTER_KW( "server_control_force_after" );
    const std::string SERVER_CONTROL_WAIT_FOREVER_KW( "server_control_wait_forever" );

    const std::string SERVER_CONTROL_SHUTDOWN( "server_control_shutdown" );
    const std::string SERVER_CONTROL_PAUSE( "server_control_pause" );
    const std::string SERVER_CONTROL_RESUME( "server_control_resume" );
    const std::string SERVER_CONTROL_STATUS( "server_control_status" );

    const std::string SERVER_CONTROL_ALL_OPT( "all" );
    const std::string SERVER_CONTROL_HOSTS_OPT( "hosts" );
    const std::string SERVER_CONTROL_SUCCESS( "server_control_success" );


    const std::string SERVER_PAUSED_ERROR( "The server is Paused, resume before issuing any other commands" );


    // this is a hand-chosen polling time for the control plane
    static const size_t SERVER_CONTROL_POLLING_TIME_MILLI_SEC = 500;

    // derived from above - used to wait for the server to shut down or resume
    static const size_t SERVER_CONTROL_FWD_SLEEP_TIME_MILLI_SEC = SERVER_CONTROL_POLLING_TIME_MILLI_SEC / 4.0;

    class server_control_executor {
        public:
            // @brief constructor
            server_control_executor( ) {}
            server_control_executor( const std::string& ); // port property

            // @brief callable operator for thread work
            void operator()();

        private:
            // typedefs
            typedef boost::function< error( const std::string&, const size_t, std::string& ) > ctrl_func_t;
            typedef std::vector< std::string >                host_list_t;

            // members
            server_control_executor( const server_control_executor& ) {}
            error process_operation(
                const zmq::message_t&, // incoming msg
                std::string& );        // outgoing text

            error extract_command_parameters(
                const irods::control_plane_command&, // incoming command
                std::string&,                        // command name
                std::string&,                        // command option
                std::string&,                        // wait option
                size_t&,                             // wait time in seconds
                host_list_t& ); // hostnames
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
            error notify_icat_and_local_servers_preop(
                const std::string&, // command
                const std::string&, // command option
                const std::string&, // wait option
                const size_t&,      // wait seconds
                const host_list_t&, // irods hostnames
                std::string& );     // output
            error notify_icat_and_local_servers_postop(
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

            // attributes
            const std::string port_prop_;
            boost::unordered_map< std::string, ctrl_func_t >  op_map_;
            std::string my_host_name_;
            std::string icat_host_name_;

    }; // class server_control_executor

    class server_control_plane {
        public:
            // @brief default constructor taking the port property
            server_control_plane( const std::string& );

            // @brief destructor
            ~server_control_plane();

        private:

            // @brief disallow copy constructor
            server_control_plane( server_control_plane& ) {}

            // @brief functor which manages the control
            server_control_executor control_executor_;

            // @brief thread which manages the control loop
            boost::thread control_thread_;

    }; // server_control_plane

}; // namespace irods

#endif // SERVER_CONTROL_PLANE_HPP




