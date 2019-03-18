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

#include "json.hpp"

#include <boost/format.hpp>

using json = nlohmann::json;

int _rsZoneReport( rsComm_t* _comm, bytesBuf_t** _bbuf );

int rsZoneReport( rsComm_t* _comm, bytesBuf_t** _bbuf ) {
    rodsServerHost_t* rods_host;
    int status = getAndConnRcatHost(_comm, MASTER_RCAT, nullptr, &rods_host);
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

        if ( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            status = _rsZoneReport( _comm, _bbuf );
        }
        else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
            status = SYS_NO_RCAT_SERVER_ERR;
        }
        else {
            rodsLog( LOG_ERROR, "role not supported [%s]", svc_role.c_str() );
            status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
        }
    }
    else {
        status = rcZoneReport( rods_host->conn, _bbuf );
    }

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "rsZoneReport: rcZoneReport failed, status = %d", status );
    }

    return status;
} // rsZoneReport

static void report_server_connect_error(
    int                _error,
    const std::string& _host_name,
    const std::string& _resc_name,
    json&              _resc_arr )
{
    std::stringstream msg;
    msg << "failed in svrToSvrConnect for resource ["
        << _resc_name
        << "], at hostname ["
        << _host_name
        << "], error code "
        << _error;

    irods::error err = ERROR(_error, msg.str());
    irods::log( err );

    _resc_arr.push_back({"ERROR", msg});
}

static
irods::error get_server_reports(rsComm_t* _comm, json& _resc_arr)
{
    _resc_arr = json::array();

    std::map< rodsServerHost_t*, int > svr_reported;
    rodsServerHost_t* icat_host = 0;
    char* zone_name = getLocalZoneName();
    int status = getRcatHost( MASTER_RCAT, zone_name, &icat_host );
    if ( status < 0 ) {
        return ERROR(status, "getRcatHost failed");
    }

    for (irods::resource_manager::iterator itr = resc_mgr.begin();
         itr != resc_mgr.end();
         ++itr)
    {
        irods::resource_ptr resc = itr->second;

        rodsServerHost_t* tmp_host = 0;
        irods::error ret = resc->get_property< rodsServerHost_t* >( irods::RESOURCE_HOST, tmp_host );
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
        ret = resc->get_property< std::string >( irods::RESOURCE_NAME, resc_name );
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
        status = rcServerReport( tmp_host->conn, &bbuf );
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
        std::string tmp_str = bbuf ? std::string((char*) bbuf->buf, bbuf->len) : std::string();

        try {
            _resc_arr.push_back(json::parse(tmp_str));
            freeBBuf(bbuf);
        }
        catch (const json::type_error& e) {
            std::string msg = "json_loads failed [";
            msg += e.what();
            msg += "]";
            irods::log(ERROR(ACTION_FAILED_ERR, msg));
        }
    } // for itr

    return SUCCESS();
} // get_server_reports

static
irods::error get_coordinating_resources(rsComm_t* _comm, json& _resources)
{
    _resources = json::array();

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

        auto entry = json::object();

        ret = serialize_resource_plugin_to_json(itr->second, entry);
        if(!ret.ok()) {
            ret = PASS(ret);
            irods::log(ret);
            entry["ERROR"] = ret.result();
        }

        _resources.push_back(entry);
    } // for itr

    return SUCCESS();
} // get_coordinating_resources

int _rsZoneReport(rsComm_t* _comm, bytesBuf_t** _bbuf)
{
    bytesBuf_t* bbuf = 0;
    int status = rsServerReport(_comm, &bbuf);
    if ( status < 0 ) {
        freeBBuf(bbuf);
        rodsLog(LOG_ERROR, "_rsZoneReport - rsServerReport failed %d", status);
        return status;
    }

    json cat_svr;

    try {
        cat_svr = json::parse(std::string(static_cast<char*>(bbuf->buf), bbuf->len));
        freeBBuf(bbuf);
    }
    catch (const json::type_error& e) {
        rodsLog(LOG_ERROR, "_rsZoneReport - json::parse failed [%s]", e.what());
        return ACTION_FAILED_ERR;
    }

    json coord_resc;
    irods::error ret = get_coordinating_resources( _comm, coord_resc );
    if ( !ret.ok() ) {
        rodsLog(LOG_ERROR, "_rsZoneReport - get_server_reports failed, status = %d", ret.code());
        return ret.code();
    }

    json svr_arr;
    ret = get_server_reports( _comm, svr_arr );
    if ( !ret.ok() ) {
        rodsLog(LOG_ERROR, "_rsZoneReport - get_server_reports failed, status = %d", ret.code());
        return ret.code();
    }

    cat_svr["coordinating_resources"] = coord_resc;

    auto zone_obj = json::object();
    zone_obj["servers"] = svr_arr;
    zone_obj["icat_server"] = cat_svr;

    auto zone_arr = json::array();
    zone_arr.push_back(zone_obj);

    auto zone = json::object();
    zone["schema_version"] = (boost::format("%s/%s/zone_bundle.json")
         % irods::get_server_property<const std::string>("schema_validation_base_uri")
         % irods::get_server_property<const std::string>("schema_version")).str();

    zone["zones"] = zone_arr;

    const auto zr = zone.dump(4);
    char* tmp_buf = new char[zr.length() + 1]{};
    std::strncpy(tmp_buf, zr.c_str(), zr.length());

    *_bbuf = (bytesBuf_t*) malloc(sizeof(bytesBuf_t));
    if (!*_bbuf) {
        delete [] tmp_buf;
        rodsLog( LOG_ERROR, "_rsZoneReport: failed to allocate _bbuf" );
        return SYS_MALLOC_ERR;
    }

    (*_bbuf)->buf = tmp_buf;
    (*_bbuf)->len = zr.length();

    return 0;
} // _rsZoneReport
