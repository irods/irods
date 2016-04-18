

#include "rodsErrorTable.h"
#include "irods_server_state.hpp"

namespace irods {

    const std::string server_state::RUNNING( "server_state_running" );
    const std::string server_state::PAUSED( "server_state_paused" );
    const std::string server_state::STOPPED( "server_state_stopped" );
    const std::string server_state::EXITED( "server_state_exited" );


    server_state::server_state() :
        state_( RUNNING )  {
    }

    server_state& server_state::instance() {
        static server_state instance_;
        return instance_;
    }

    error server_state::operator()( const std::string& _s ) {
        boost::mutex::scoped_lock lock( mutex_ );
        if ( RUNNING != _s &&
             PAUSED  != _s &&
             STOPPED != _s &&
             EXITED  != _s ) {
            std::string msg( "invalid state [" );
            msg += _s;
            msg += "]";
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       msg );

        }

        state_ = _s;

        return SUCCESS();
    }

    std::string server_state::operator()() {
        boost::mutex::scoped_lock lock( mutex_ );
        return state_;
    }


}; // namespace irods




