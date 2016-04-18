


#ifndef IRODS_SERVER_STATE_HPP
#define IRODS_SERVER_STATE_HPP

#include "irods_error.hpp"

#include <boost/thread/mutex.hpp>
namespace irods {
    class server_state {
        public:
            static server_state& instance();
            error operator()( const std::string& _s );
            std::string operator()();

            static const std::string RUNNING;
            static const std::string PAUSED;
            static const std::string STOPPED;
            static const std::string EXITED;

        private:
            server_state();
            server_state( server_state& ) {}
            server_state( const server_state& ) {}

            boost::mutex mutex_;
            std::string state_;

    }; // class server_state

}; // namespace irods



#endif // IRODS_SERVER_STATE_HPP


