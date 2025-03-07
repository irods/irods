#include "irods/rcMisc.h"
#include "irods/rsGlobalExtern.hpp"
#include "irods/rodsErrorTable.h"
#include "irods/miscServerFunct.hpp"
#include "irods/rsZoneReport.hpp"
#include "irods/rsServerReport.hpp"

#include "irods/irods_log.hpp"
#include "irods/zone_report.h"
#include "irods/server_report.h"
#include "irods/irods_resource_manager.hpp"
#include "irods/irods_resource_backport.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_report_plugins_in_json.hpp"

#include <nlohmann/json.hpp>

#include <boost/format.hpp>

using json = nlohmann::json;

int _rsZoneReport( rsComm_t* _comm, bytesBuf_t** _bbuf );

int rsZoneReport( rsComm_t* _comm, bytesBuf_t** _bbuf ) {
    rodsServerHost_t* rods_host;
    int status = getAndConnRcatHost(_comm, PRIMARY_RCAT, nullptr, &rods_host);
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

        if ( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            status = _rsZoneReport( _comm, _bbuf );
        }
        else if( irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
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

    _resc_arr.push_back({"ERROR", msg.str()});
}

static
irods::error get_server_reports(rsComm_t* _comm, json& _resc_arr)
{
    _resc_arr = json::array();

    std::map< rodsServerHost_t*, int > svr_reported;

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

        // skip null tmp_hosts resources ( coordinating )
        if (!tmp_host) { // NOLINT(readability-implicit-bool-conversion)
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

    auto zone_obj = json::object();
    zone_obj["coordinating_resources"] = coord_resc;
    zone_obj["servers"] = svr_arr;

    auto zone_arr = json::array();
    zone_arr.push_back(zone_obj);

    auto zone = json::object();
    zone["zones"] = zone_arr;

    const auto zr = zone.dump(4);
    const auto buf_size = sizeof(char) * (zr.length() + 1);
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
    char* tmp_buf = static_cast<char*>(std::malloc(buf_size));
    std::memset(tmp_buf, 0, buf_size);
    std::strncpy(tmp_buf, zr.c_str(), zr.length());

    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
    *_bbuf = static_cast<bytesBuf_t*>(std::malloc(sizeof(bytesBuf_t)));
    if (!*_bbuf) {
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
        std::free(tmp_buf);
        rodsLog( LOG_ERROR, "_rsZoneReport: failed to allocate _bbuf" );
        return SYS_MALLOC_ERR;
    }

    (*_bbuf)->buf = tmp_buf;
    (*_bbuf)->len = static_cast<int>(zr.length());

    return 0;
} // _rsZoneReport
