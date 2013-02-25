

// =-=-=-=-=-=-=
// irods includes
#define RODS_SERVER
#include "miscServerFunct.h"
#include "objInfo.h"
#include "dataObjCreate.h"
#include "specColl.h"
#include "collection.h"

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
        // flag to skip redirect for spec coll
        bool skip_redir_for_spec_coll = false;

        // =-=-=-=-=-=-=-
        // default to local host if there is a failure
        _out_flag = LOCAL_HOST;

        // =-=-=-=-=-=-=-
        // cache the operation, as we may need to modify it
        std::string oper = _oper;

        // =-=-=-=-=-=-=-
        // if this is a put operation then we do not have a first class object
        resource_ptr resc;
        eirods::file_object file_obj;
        
        if( eirods::EIRODS_CREATE_OPERATION != oper ) {
            // =-=-=-=-=-=-=-
            // call factory for given obj inp, get a file_object
            error fac_err = file_object_factory( _comm, _data_obj_inp, file_obj );
            if( !fac_err.ok() ) {
                std::stringstream msg;
                msg << "resource_redirect :: failed in file_object_factory";
                return PASSMSG( msg.str(), fac_err );
            }

            // =-=-=-=-=-=-=-
            // resolve a resc ptr for the given file_object
            eirods::error err = file_obj.resolve( resc_mgr, resc );
            if( !err.ok() ) {
                std::stringstream msg;
                msg << "resource_redirect :: failed in file_object.resolve";
                return PASSMSG( msg.str(), err );
            }

        } else {
            std::string orig_path = _data_obj_inp->objPath;
            std::string path = _data_obj_inp->objPath;
            size_t pos = path.find_last_of( '/' );
            if( pos != std::string::npos ) {
                path = path.substr( 0, pos );
            }

            strncpy( _data_obj_inp->objPath, path.c_str(), MAX_NAME_LEN );
            rodsObjStat_t *rodsObjStatOut = NULL;
            int spec_stat = collStat( _comm, _data_obj_inp, &rodsObjStatOut );
            strncpy( _data_obj_inp->objPath, orig_path.c_str(), MAX_NAME_LEN );

            // =-=-=-=-=-=-=-
            // if this is a spec coll, we need to short circuit the create
            // as everything needs to be in the same resource for a spec coll
            file_obj.logical_path( _data_obj_inp->objPath );
            if( spec_stat >= 0 && rodsObjStatOut->specColl != NULL ) {
                std::string resc_hier = rodsObjStatOut->specColl->rescHier;
                file_obj.resc_hier( resc_hier );
                skip_redir_for_spec_coll = true; 
            } else {
                // =-=-=-=-=-=-=-
                // check for incoming requested destination resource first
                std::string resc_name;
                char* tmp_name = 0;
                if( ( tmp_name = getValByKey( &_data_obj_inp->condInput, BACKUP_RESC_NAME_KW ) ) == NULL &&
                    ( tmp_name = getValByKey( &_data_obj_inp->condInput, DEST_RESC_NAME_KW   ) ) == NULL &&
                    ( tmp_name = getValByKey( &_data_obj_inp->condInput, DEF_RESC_NAME_KW    ) ) == NULL &&
                    ( tmp_name = getValByKey( &_data_obj_inp->condInput, RESC_NAME_KW        ) ) == NULL ) {

                    // =-=-=-=-=-=-=-
                    // this must me a 'create' opreation so we get the resource in question
                    rescGrpInfo_t* grp_info = 0;
                    int status = getRescGrpForCreate( _comm, _data_obj_inp, &grp_info ); 
                    if( status < 0 || !grp_info || !grp_info->rescInfo ) {
                        return ERROR( status, "resource_redirect - failed in getRescGrpForCreate" );
                    }
                        
                    resc_name = grp_info->rescInfo->rescName;

                    // =-=-=-=-=-=-=-
                    // clean up memory
                    delete grp_info->rescInfo;
                    delete grp_info;

                } else {
                    resc_name = tmp_name;
                }

                // =-=-=-=-=-=-=-
                // request the resource by name
                error err = resc_mgr.resolve( resc_name, resc );
                if( !err.ok() ) {
                    return PASSMSG( "resource_redirect - failed in resc_mgr.resolve", err );

                }
                
                file_obj.resc_hier( resc_name );

            }
        }
      
        // =-=-=-=-=-=-=-
        // get current hostname, which is also done by init local server host
        char host_name_char[ MAX_NAME_LEN ];
        if( gethostname( host_name_char, MAX_NAME_LEN ) < 0 ) {
            std::stringstream msg;
            msg << "resource_redirect :: failed in gethostname";
            return ERROR( -1, msg.str() );

        }
        std::string host_name( host_name_char );

        // =-=-=-=-=-=-=-
        // unholy special treatment of special collections, once again
        hierarchy_parser parser;
        if( !skip_redir_for_spec_coll ) {
            // =-=-=-=-=-=-=-
            // query the resc given the operation for a hier string which 
            // will determine the host
            float            vote = 0.0;
            error err = resc->call< const std::string*, const std::string*, eirods::hierarchy_parser*, float* >( 
                              _comm, "redirect", &file_obj, &oper, &host_name, &parser, &vote );
            // =-=-=-=-=-=-=-
            // extract the hier string from the parser, politely.
            parser.str( _out_resc_hier ); 
            if( !err.ok() || 0.0 == vote ) {
                std::stringstream msg;
                msg << "resource_redirect :: failed in resc.call( redirect ) ";
                msg << "host [" << host_name << "] ";
                msg << "hier [" << _out_resc_hier      << "]";
                return PASSMSG( msg.str(), err );
            }
        
        } else {
            parser.set_string( file_obj.resc_hier() );
            _out_resc_hier = file_obj.resc_hier();

        }
        
        std::string last_resc;
        parser.last_resc( last_resc );
        
        // =-=-=-=-=-=-=-
        // get the host property from the last resc and get the
        // host name from that host
        rodsServerHost_t* last_resc_host;
        eirods::error err = get_resource_property< rodsServerHost_t* >( last_resc, "host", last_resc_host ); 
        if( !err.ok() ) {
            std::stringstream msg;
            msg << "resource_redirect :: failed in get_resource_property call ";
            msg << "for [" << last_resc << "]";
            return PASSMSG( msg.str(), err );
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
            _out_resc_hier = _out_resc_hier;
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
        _out_host      = last_resc_host;
        _out_flag      = REMOTE_HOST;
        return SUCCESS();

    } // resource_redirect

}; // namespace eirods



