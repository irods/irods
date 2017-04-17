#include "rcMisc.h"
#include "rsGlobalExtern.hpp"
#include "rodsErrorTable.h"
#include "miscServerFunct.hpp"
#include "rsZoneReport.hpp"
#include "rsServerReport.hpp"

#include "irods_log.hpp"
#include "zone_report.h"
#include "server_report.h"
#include "irods_resource_manager.hpp"
#include "irods_resource_backport.hpp"
#include "irods_server_properties.hpp"
#include "irods_configuration_keywords.hpp"
#include "irods_report_plugins_in_json.hpp"

#include "jansson.h"
#include <boost/format.hpp>



int _rsZoneReport(
    rsComm_t*    _comm,
    bytesBuf_t** _bbuf );

int rsZoneReport(
    rsComm_t*    _comm,
    bytesBuf_t** _bbuf ) {
    rodsServerHost_t* rods_host;
    int status = getAndConnRcatHost(
                     _comm,
                     MASTER_RCAT,
                     ( const char* )NULL,
                     &rods_host );
    if ( status < 0 ) {
        return status;
    }

    if ( rods_host->localFlag == LOCAL_HOST ) {
        std::string svc_role;
        irods::error ret = get_catalog_service_role(svc_role);
        if(!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }

        if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            status = _rsZoneReport(
                         _comm,
                         _bbuf );
        } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
            status = SYS_NO_RCAT_SERVER_ERR;
        } else {
            rodsLog(
                LOG_ERROR,
                "role not supported [%s]",
                svc_role.c_str() );
            status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
        }
    }
    else {
        status = rcZoneReport( rods_host->conn,
                               _bbuf );
    }

    if ( status < 0 ) {
        rodsLog(
            LOG_ERROR,
            "rsZoneReport: rcZoneReport failed, status = %d",
            status );
    }

    return status;

} // rsZoneReport

static
void report_server_connect_error(
    int                _error,
    const std::string& _host_name,
    const std::string& _resc_name,
    json_t*&           _resc_arr ) {

    std::stringstream msg;
    msg << "failed in svrToSvrConnect for resource ["
        << _resc_name
        << "], at hostname ["
        << _host_name
        << "], error code "
        << _error;

    irods::error err = ERROR(_error, msg.str());
    irods::log( err );

    json_t* obj = json_object();
    if(!obj) {
        rodsLog(
            LOG_ERROR,
            "%s:%d - failed to create json object",
            __FUNCTION__, __LINE__);
    }

    json_object_set_new(obj, "ERROR", json_string(msg.str().c_str()));
    json_array_append_new( _resc_arr, obj );

}

static
irods::error get_server_reports(
    rsComm_t* _comm,
    json_t*&  _resc_arr ) {

    _resc_arr = json_array();
    if ( !_resc_arr ) {
        return ERROR(
                   SYS_MALLOC_ERR,
                   "json_object() failed" );
    }

    std::map< rodsServerHost_t*, int > svr_reported;
    rodsServerHost_t* icat_host = 0;
    char* zone_name = getLocalZoneName();
    int status = getRcatHost( MASTER_RCAT, zone_name, &icat_host );
    if ( status < 0 ) {
        return ERROR(
                   status,
                   "getRcatHost failed" );
    }

    for ( irods::resource_manager::iterator itr = resc_mgr.begin();
            itr != resc_mgr.end();
            ++itr ) {

        irods::resource_ptr resc = itr->second;

        rodsServerHost_t* tmp_host = 0;
        irods::error ret = resc->get_property< rodsServerHost_t* >(
                               irods::RESOURCE_HOST,
                               tmp_host );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            continue;
        }

        // skip the icat server as that is done separately
        // also skip null tmp_hosts resources ( coordinating )
        // skip local host
        if ( !tmp_host || tmp_host == icat_host || LOCAL_HOST == tmp_host->localFlag ) {
            continue;

        }

        // skip previously reported servers
        std::map< rodsServerHost_t*, int >::iterator svr_itr =
            svr_reported.find( tmp_host );
        if ( svr_itr != svr_reported.end() ) {
            continue;

        }

        std::string resc_name;
        ret = resc->get_property< std::string >(
                  irods::RESOURCE_NAME,
                  resc_name );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            continue;
        }

        svr_reported[ tmp_host ] = 1;

        int status = svrToSvrConnect( _comm, tmp_host );
        if ( status < 0 ) {
            report_server_connect_error(
                status,
                tmp_host->hostName->name,
                resc_name,
                _resc_arr);
            continue;
        }

        bytesBuf_t* bbuf = NULL;
        status = rcServerReport(
                     tmp_host->conn,
                     &bbuf );
        if ( status < 0 ) {
            freeBBuf( bbuf );
            bbuf = NULL;
            rodsLog(
                LOG_ERROR,
                "rcServerReport failed for [%s], status = %d",
                tmp_host->hostName->name,
                status );
        }

        // possible null termination issues
        std::string tmp_str = bbuf ? std::string( ( char* )bbuf->buf, bbuf->len ) :
                              std::string();

        json_error_t j_err;
        json_t* j_resc = json_loads(
                             tmp_str.data(),
                             JSON_REJECT_DUPLICATES,
                             &j_err );

        freeBBuf( bbuf );
        if ( !j_resc ) {
            std::string msg( "json_loads failed [" );
            msg += j_err.text;
            msg += "]";
            irods::log( ERROR(
                            ACTION_FAILED_ERR,
                            msg ) );
            continue;
        }

        json_array_append_new( _resc_arr, j_resc );

    } // for itr

    return SUCCESS();

} // get_server_reports

static
irods::error get_coordinating_resources(
    rsComm_t* _comm,
    json_t*&  _resources ) {

    _resources = json_array();
    if ( !_resources ) {
        return ERROR(
                   SYS_MALLOC_ERR,
                   "json_object() failed" );
    }

    for( auto itr = std::begin(resc_mgr); itr != std::end(resc_mgr); ++itr ) {
        irods::resource_ptr& resc = itr->second;

        rodsServerHost_t* tmp_host = nullptr;
        irods::error ret = resc->get_property< rodsServerHost_t* >(
                               irods::RESOURCE_HOST,
                               tmp_host );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            continue;
        }

        // skip non-null hosts ( not coordinating )
        if ( tmp_host ) {
            continue;

        }

        json_t* entry = json_object();
        if ( !entry ) {
            return ERROR(
                       SYS_MALLOC_ERR,
                       "failed to alloc entry" );
        }

        ret = serialize_resource_plugin_to_json(itr->second, entry);
        if(!ret.ok()) {
            ret = PASS(ret);
            irods::log(ret);
            json_object_set_new(entry, "ERROR", json_string(ret.result().c_str()));
        }

        json_array_append_new( _resources, entry );

    } // for itr

    return SUCCESS();

} // get_coordinating_resources

int _rsZoneReport(
    rsComm_t*    _comm,
    bytesBuf_t** _bbuf ) {

    bytesBuf_t* bbuf = 0;
    int status = rsServerReport(
                     _comm,
                     &bbuf );
    if ( status < 0 ) {
        freeBBuf( bbuf );
        rodsLog(
            LOG_ERROR,
            "_rsZoneReport - rsServerReport failed %d",
            status );
        return status;
    }

    json_error_t j_err;
    json_t* cat_svr = json_loads( ( char* )bbuf->buf, JSON_REJECT_DUPLICATES, &j_err );
    freeBBuf( bbuf );
    if ( !cat_svr ) {
        rodsLog(
            LOG_ERROR,
            "_rsZoneReport - json_loads failed [%s]",
            j_err.text );
        return ACTION_FAILED_ERR;
    }

    json_t* coord_resc = 0;
    irods::error ret = get_coordinating_resources( _comm, coord_resc );
    if ( !ret.ok() ) {
        rodsLog(
            LOG_ERROR,
            "_rsZoneReport - get_server_reports failed, status = %d",
            ret.code() );
        return ret.code();
    }

    json_t* svr_arr = 0;
    ret = get_server_reports( _comm, svr_arr );
    if ( !ret.ok() ) {
        rodsLog(
            LOG_ERROR,
            "_rsZoneReport - get_server_reports failed, status = %d",
            ret.code() );
        return ret.code();
    }

    json_t* zone_obj = json_object();
    if ( !zone_obj ) {
        rodsLog(
            LOG_ERROR,
            "failed to allocate json_object" );
        return SYS_MALLOC_ERR;
    }

    json_object_set_new( zone_obj, "servers", svr_arr );
    json_object_set_new( cat_svr, "coordinating_resources", coord_resc );
    json_object_set_new( zone_obj, "icat_server", cat_svr );

    json_t* zone_arr = json_array();
    if ( !zone_arr ) {
        rodsLog(
            LOG_ERROR,
            "failed to allocate json_array" );
        return SYS_MALLOC_ERR;
    }

    json_array_append_new( zone_arr, zone_obj );

    json_t* zone = json_object();
    if ( !zone ) {
        rodsLog(
            LOG_ERROR,
            "failed to allocate json_object" );
        return SYS_MALLOC_ERR;
    }

    json_object_set_new(
        zone,
        "schema_version",
        json_string((boost::format("%s/%s/zone_bundle.json") %
             irods::get_server_property<const std::string>("schema_validation_base_uri") %
             irods::get_server_property<const std::string>("schema_version")).str().c_str()
            )
        );
    json_object_set_new( zone, "zones", zone_arr );

    char* tmp_buf = json_dumps( zone, JSON_INDENT( 4 ) );
    json_decref( zone );

    ( *_bbuf ) = ( bytesBuf_t* ) malloc( sizeof( bytesBuf_t ) );
    if ( !( *_bbuf ) ) {
        rodsLog(
            LOG_ERROR,
            "_rsZoneReport: failed to allocate _bbuf" );
        return SYS_MALLOC_ERR;

    }

    ( *_bbuf )->buf = tmp_buf;
    ( *_bbuf )->len = strlen( tmp_buf );

    return 0;

} // _rsZoneReport
