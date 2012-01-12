


#ifndef __SOCKET_WRAPPER_HPP__
#define __SOCKET_WRAPPER_HPP__

// =-=-=-=-=-=-=-
// Boost Includes
#include <boost/asio.hpp>

namespace irods {

    // =-=-=-=-=-=-=-
    // JMC :: simple wrapper for boost::asio sockets.  they did
    //     :: not provide a pure interface base class wich didnt
    //     :: have associated template parameters so one needed to
    //     :: be provided.  unfortuately the inteface needs redefined.
    class socket_wrapper { 

    public:
        // =-=-=-=-=-=-=-
        // members
        socket_wrapper() {}
        virtual ~socket_wrapper() {}

        // =-=-=-=-=-=-=-
        // interface declaration
        virtual bool open( int, const char* ) = 0;

    }; // class socket_wrapper

    // =-=-=-=-=-=-=-
    // derived class for handling tcp style sockets
    class socket_wrapper_tcp : public socket_wrapper {
        boost::asio::ip::tcp::socket* sock_;
    public:
        // =-=-=-=-=-=-=-
        // members
        socket_wrapper_tcp();
        ~socket_wrapper_tcp();
        virtual bool open( int, char* );

    }; // class socket_wrapper_tcp 

    // =-=-=-=-=-=-=-
    // derived class for handling udp style sockets
    class socket_wrapper_udp : public socket_wrapper {
        boost::asio::ip::udp::socket* sock_;
    public:
        // =-=-=-=-=-=-=-
        // members
        socket_wrapper_udp();
        ~socket_wrapper_udp();
        virtual bool open( int, const char* );

    }; // class socket_wrapper_udp 

}; // namespace irods


#endif //  __SOCKET_WRAPPER_HPP__



