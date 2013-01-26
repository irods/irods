

// =-=-=-=-=-=-=
// irods includes
#include "miscServerFunct.h"

// =-=-=-=-=-=-=
// eirods includes
#include "eirods_resource_redirect.h"
#include "eirods_hierarchy_parser.h"
#include "eirods_resource_backport.h"

namespace eirods {

    // =-=-=-=-=-=-=-
    // static function to query resource for chosen server to which to redirect
    // for a given operation
    error resource_redirect( const std::string&   _oper,
                             rsComm_t*            _comm,
                             dataObjInp_t*        _data_obj_inp, 
                             std::string&         _out_resc_hier,
                             rodsServerHost_t*&   _out_host, 
                             int&                 _out_flag ) {
        // =-=-=-=-=-=-=-
        // default to local host if there is a failure
        _out_flag = LOCAL_HOST;

        // =-=-=-=-=-=-=-
        // call factory for given obj inp, get a file_object
        file_object file_obj;
        error ret = file_object_factory( _comm, _data_obj_inp, file_obj );
        if( !ret.ok() ) {
            std::stringstream msg;
            msg << "resource_redirect :: failed in file_object_factory";
            return PASSMSG( msg.str(), ret );
        }

        // =-=-=-=-=-=-=-
        // resolve a resc ptr for the given file_object
        resource_ptr resc;
        ret = file_obj.resolve( resc_mgr, resc );
        if( !ret.ok() ) {
            std::stringstream msg;
            msg << "resource_redirect :: failed in file_object.resolve";
            return PASSMSG( msg.str(), ret );
        }

        // =-=-=-=-=-=-=-
        // get current hostname, which is also dont by init local server host
        char host_name_char[ MAX_NAME_LEN ];
        if( gethostname( host_name_char, MAX_NAME_LEN ) < 0 ) {
            std::stringstream msg;
            msg << "resource_redirect :: failed in gethostname";
            return ERROR( -1, msg.str() );

        }
        std::string host_name( host_name_char );

        // =-=-=-=-=-=-=-
        // query the resc given the operation for a hier string which 
        // will determine the host
        float       vote = 0.0;
        std::string hier;
        ret = resc->call< const std::string*, const std::string*, std::string*, float* >( 
                          _comm, "redirect", &file_obj, &_oper, &host_name, &hier, &vote );
        if( !ret.ok() || 0.0 == vote ) {
            std::stringstream msg;
            msg << "resource_redirect :: failed in resc.call( redirect ) ";
            msg << "host [" << host_name << "] ";
            msg << "hier [" << hier      << "]";
            return PASSMSG( msg.str(), ret );
        }

        // =-=-=-=-=-=-=-
        // pull out the last string in the hier string
        eirods::hierarchy_parser parser;
        parser.set_string( hier );

        std::string last_resc;
        parser.last_resc( last_resc );

        // =-=-=-=-=-=-=-
        // get the host property from the last resc and get the
        // host name from that host
        rodsServerHost_t* last_resc_host;
        ret = get_resource_property< rodsServerHost_t* >( last_resc, "host", last_resc_host ); 
        if( !ret.ok() ) {
            std::stringstream msg;
            msg << "resource_redirect :: failed in get_resource_property call ";
            msg << "for [" << last_resc << "]";
            return PASSMSG( msg.str(), ret );
        }

        // =-=-=-=-=-=-=
        // iterate over the list of hostName_t* and see if any match our
        // host name.  if we do, then were local
        bool        match_flg = false;
        hostName_t* tmp_host  = last_resc_host->hostName;
        while( tmp_host ) {
            std::string name( tmp_host->name );
            if( name.find( host_name ) != std::string::npos ) {
                match_flg = true;
                break;

            } 

            tmp_host = tmp_host->next;

        } // while tmp_host

        // =-=-=-=-=-=-=-
        // are we really, really local?
        if( match_flg ) {
            _out_resc_hier = hier;
            _out_flag = LOCAL_HOST;
            _out_host = 0;
            return SUCCESS();
        }

        // =-=-=-=-=-=-=-
        // it was not a local resource so then do a svr to svr connection
        int conn_err = svrToSvrConnect( _comm, last_resc_host );
        if( conn_err < 0 ) {
            std::stringstream msg;
            msg << "resource_redirect :: failed in svrToSvrConnect";
            return ERROR( conn_err, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // return with a hier string and new connection as remote host
        _out_resc_hier = hier;
        _out_host      = last_resc_host;
        _out_flag      = REMOTE_HOST;
        return SUCCESS();

    } // resource_redirect

}; // namespace eirods



