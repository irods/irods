#include "rsGlobalExtern.hpp"
#include "rodsErrorTable.h"
#include "miscServerFunct.hpp"
#include "reIn2p3SysRule.hpp"
#include "irods_log.hpp"
#include "irods_plugin_name_generator.hpp"
#include "irods_resource_manager.hpp"
#include "irods_get_full_path_for_config_file.hpp"
#include "server_report.h"
#include "irods_server_properties.hpp"
#include "irods_environment_properties.hpp"
#include "irods_load_plugin.hpp"
#include "irods_report_plugins_in_json.hpp"
#include "rsServerReport.hpp"
#include <unistd.h>
#include <grp.h>

#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>

#include "json.hpp"

#include <sys/utsname.h>

namespace fs = boost::filesystem;

using json = nlohmann::json;

int _rsServerReport( rsComm_t* _comm, bytesBuf_t** _bbuf );

int rsServerReport( rsComm_t* _comm, bytesBuf_t** _bbuf )
{
    // always execute this locally
    int status = _rsServerReport( _comm, _bbuf );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "rsServerReport: rcServerReport failed, status = %d", status );
    }

    return status;
} // rsServerReport

json make_federation_set( const std::vector< std::string >& _feds )
{
    if ( _feds.empty() ) {
        return nullptr;
    }

    auto array = json::array();

    for ( const auto& federation : _feds ) {
        std::vector<std::string> zone_sid_vals;
        boost::split( zone_sid_vals, federation, boost::is_any_of( "-" ) );

        if ( zone_sid_vals.size() > 2 ) {
            rodsLog( LOG_ERROR, "multiple hyphens found in RemoteZoneSID [%s]", federation.c_str() );
            continue;
        }

        array.push_back(json::object({
            {"zone_name", zone_sid_vals[0]},
            {"zone_key", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"},
            {"negotiation_key", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"}
        }));
    } // for i

    return array;
} // make_federation_set

irods::error sanitize_server_config_keys( json& _svr_cfg )
{
    if ( _svr_cfg.is_null() ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "null json object" );
    }

    // sanitize the top level keys
    _svr_cfg["zone_key"] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    _svr_cfg["negotiation_key"] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    _svr_cfg["server_control_plane_key"] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

    // return if federation is not available
    if (!_svr_cfg.count("federation")) {
        return SUCCESS();
    }

    // get the federation object and sanitize all federation keys
    for (auto&& o : _svr_cfg["federation"]) {
        o["negotiation_key"] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
        o["zone_key"] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    }

    return SUCCESS();
} // sanitize_server_config_keys

namespace
{
    irods::error load_json_file(const std::string& _file, json& _json)
    {
        std::ifstream in{_file};

        if (!in) {
            std::string msg( "failed to load file [" );
            msg += _file;
            msg += ']';
            return ERROR( -1, msg );
        }

        try {
            in >> _json;
        }
        catch (const json::parse_error& e) {
            return ERROR(-1, boost::format("failed to parse json [file=%s, error=%s].") % _file % e.what());
        }

        return SUCCESS();
    }
}

irods::error convert_server_config( json& _svr_cfg )
{
    // =-=-=-=-=-=-=-
    // if json file exists, simply load that
    std::string svr_cfg;
    irods::error ret = irods::get_full_path_for_config_file( "server_config.json", svr_cfg );
    if ( !ret.ok() || !fs::exists( svr_cfg ) ) {
        return ERROR(SYS_CONFIG_FILE_ERR, boost::format("Could not find server config file at [%s]") % svr_cfg);
    }

    if (auto err = load_json_file(svr_cfg, _svr_cfg); !err.ok()) {
        return err;
    }

    return sanitize_server_config_keys( _svr_cfg );
} // convert_server_config

irods::error convert_host_access_control( json& _host_ctrl )
{
    std::string cfg_file;
    irods::error ret = irods::get_full_path_for_config_file( HOST_ACCESS_CONTROL_FILE, cfg_file );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    return load_json_file(cfg_file, _host_ctrl);
} // convert_host_access_control

irods::error convert_irods_host( json& _irods_host )
{
    std::string cfg_file;
    irods::error ret = irods::get_full_path_for_config_file( HOST_CONFIG_FILE, cfg_file );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    return load_json_file(cfg_file, _irods_host);
} // convert_irods_host

irods::error convert_service_account( json& _svc_acct )
{
    std::string env_file_path;
    std::string session_file_path;
    irods::error ret = irods::get_json_environment_file(env_file_path, session_file_path);
    if (!ret.ok()) {
        return PASS(ret);
    }

    if (!fs::exists(env_file_path)) {
        return ERROR(SYS_CONFIG_FILE_ERR, boost::format("Could not find environment file at [%s]") % env_file_path);
    }

    if (const auto err = load_json_file(env_file_path, _svc_acct); !err.ok()) {
        return err;
    }

    // sanitize the keys
    _svc_acct["irods_server_control_plane_key"] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

    return SUCCESS();
} // convert_service_account

irods::error get_uname_string( std::string& _str )
{
    struct utsname os_name;
    memset( &os_name, 0, sizeof( os_name ) );
    const int status = uname( &os_name );
    if ( status != 0 ) {
        return ERROR( status, "uname failed" );
    }

    _str.clear();
    _str += "SYS_NAME=" ;
    _str += os_name.sysname;
    _str += ";NODE_NAME=";
    _str += os_name.nodename;
    _str += ";RELEASE=";
    _str += os_name.release;
    _str += ";VERSION=";
    _str += os_name.version;
    _str += ";MACHINE=";
    _str += os_name.machine;

    return SUCCESS();
} // get_uname_string

irods::error get_user_name_string(std::string& user_name_string)
{
    uid_t uid = getuid();
    passwd *pw = getpwuid(uid);
    if (pw==nullptr) {
        return ERROR(SYS_INTERNAL_NULL_INPUT_ERR, boost::format("getpwuid() returned null for uid [%d]") % uid);
    }
    user_name_string = pw->pw_name;
    return SUCCESS();
}

irods::error get_group_name_string(std::string& group_name_string)
{
    uid_t uid = getuid();
    passwd *pw = getpwuid(uid);
    if (pw==nullptr) {
        return ERROR(SYS_INTERNAL_NULL_INPUT_ERR, boost::format("getpwuid() returned null for uid [%d]") % uid);
    }
    group *grp = getgrgid(pw->pw_gid);
    if (grp==nullptr) {
        return ERROR(SYS_INTERNAL_NULL_INPUT_ERR, boost::format("getgrgid() returned null for gid [%d]") % pw->pw_gid);
    }
    group_name_string = grp->gr_name;
    return SUCCESS();
}

irods::error get_host_system_information(json& _host_system_information)
{
    _host_system_information = json::object();

    std::string user_name_string;
    irods::error ret = get_user_name_string(user_name_string);
    if (ret.ok()) {
        _host_system_information["service_account_user_name"] = user_name_string;
    } else {
        irods::log(ret);
        _host_system_information["service_account_user_name"] = nullptr;
    }

    std::string group_name_string;
    ret = get_group_name_string(group_name_string);
    if (ret.ok()) {
        _host_system_information["service_account_group_name"] = group_name_string;
    } else {
        irods::log(ret);
        _host_system_information["service_account_group_name"] = nullptr;
    }

    char hostname_buf[512];
    const int gethostname_ret = gethostname(hostname_buf, sizeof(hostname_buf));
    if (gethostname_ret != 0) {
        rodsLog(LOG_ERROR, "get_host_system_information: gethostname() failed [%d]", errno);
        _host_system_information["hostname"] = nullptr;
    } else {
        _host_system_information["hostname"] = hostname_buf;
    }

    std::string uname_string;
    ret = get_uname_string( uname_string );
    if ( ret.ok() ) {
        _host_system_information["uname"] = uname_string;
    } else {
        irods::log( PASS( ret ) );
        _host_system_information["uname"] = nullptr;
    }

    std::vector<std::string> args;
    args.push_back( "os_distribution_name" );
    std::string os_distribution_name;
    ret = get_script_output_single_line( "python", "system_identification.py", args, os_distribution_name );
    if ( ret.ok() ) {
        _host_system_information["os_distribution_name"] = os_distribution_name;
    } else {
        irods::log( PASS( ret ) );
        _host_system_information["os_distribution_name"] = nullptr;
    }

    args.clear();
    args.push_back( "os_distribution_version" );
    std::string os_distribution_version;
    ret = get_script_output_single_line( "python", "system_identification.py", args, os_distribution_version );
    if (ret.ok()) {
        _host_system_information["os_distribution_version"] = os_distribution_version;
    } else {
        irods::log( PASS( ret ) );
        _host_system_information["os_distribution_version"] = nullptr;
    }

    return SUCCESS();
} // get_host_system_information

irods::error get_resource_array( json& _resources )
{
    _resources = json::array();

    rodsEnv my_env;
    int status = getRodsEnv( &my_env );
    if ( status < 0 ) {
        return ERROR( status, "failed in getRodsEnv" );
    }

    const std::string local_host_name = my_env.rodsHost;

    for ( irods::resource_manager::iterator itr = resc_mgr.begin(); itr != resc_mgr.end(); ++itr ) {
        irods::resource_ptr resc = itr->second;

        rodsServerHost_t* tmp_host = nullptr;
        irods::error ret = resc->get_property< rodsServerHost_t* >(
                               irods::RESOURCE_HOST,
                               tmp_host );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            continue;
        }

        // do not report coordinating resources, done elsewhere
        if ( !tmp_host ) {
            continue;
        }

        if ( LOCAL_HOST != tmp_host->localFlag ) {
            continue;
        }

        std::string host_name;
        ret = resc->get_property< std::string >( irods::RESOURCE_LOCATION, host_name );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            continue;
        }

        std::string name;
        ret = resc->get_property< std::string >( irods::RESOURCE_NAME, name );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            continue;
        }

        if ( host_name != irods::EMPTY_RESC_HOST &&
                std::string::npos == host_name.find( local_host_name ) &&
                std::string::npos == local_host_name.find( host_name ) ) {
            rodsLog(
                LOG_DEBUG,
                "get_resource_array - skipping non-local resource [%s] on [%s]",
                name.c_str(),
                host_name.c_str() );
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
} // get_resource_array

irods::error get_file_contents( const std::string _fn, std::string& _cont )
{
    std::ifstream f( _fn.c_str() );
    std::stringstream ss;
    ss << f.rdbuf();
    f.close();

    std::string in_s = ss.str();

    namespace bitr = boost::archive::iterators;
    std::stringstream o_str;
    typedef
    bitr::base64_from_binary < // convert binary values to base64 characters
    bitr::transform_width <   // retrieve 6 bit integers from a sequence of 8 bit bytes
    const char *,
          6,
          8
          >
          >
          base64_text; // compose all the above operations in to a new iterator

    std::copy(
        base64_text( in_s.c_str() ),
        base64_text( in_s.c_str() + in_s.size() ),
        bitr::ostream_iterator<char>( o_str )
    );

    _cont = o_str.str();

    size_t pad = in_s.size() % 3;
    _cont.insert( _cont.size(), ( 3 - pad ) % 3, '=' );

    return SUCCESS();
} // get_file_contents

irods::error get_config_dir( json& _cfg_dir )
{
    namespace fs = boost::filesystem;

    _cfg_dir = json::object();

    auto files = json::object();
    std::string cfg_file;
    irods::error ret = irods::get_full_path_for_config_file( irods::SERVER_CONFIG_FILE, cfg_file );

    fs::path p( cfg_file );
    std::string config_dir = p.parent_path().string();

    _cfg_dir["path"] = config_dir;

    for ( fs::directory_iterator itr( config_dir );
          itr != fs::directory_iterator();
          ++itr )
    {
        if ( fs::is_regular_file( itr->path() ) ) {
            const fs::path& p = itr->path();
            const std::string name = p.string();

            if (std::string::npos != name.find(irods::SERVER_CONFIG_FILE) ||
                std::string::npos != name.find(HOST_CONFIG_FILE) ||
                std::string::npos != name.find(HOST_ACCESS_CONTROL_FILE))
            {
                continue;
            }

            std::string contents;
            ret = get_file_contents( name, contents );

            if ( !ret.ok() ) {
                irods::log( PASS( ret ) );
                continue;
            }

            files[name] = contents;
        }
    } // for itr

    _cfg_dir["files"] = files;

    return SUCCESS();
} // get_config_dir

irods::error load_version_file( json& _version )
{
    // =-=-=-=-=-=-=-
    // if json file exists, simply load that
    fs::path version_file;
    try {
        version_file = irods::get_irods_home_directory();
    } catch (const irods::exception& e) {
        irods::log(e);
        return ERROR(-1, "failed to get irods home directory");
    }
    version_file.append("VERSION.json");

    if ( fs::exists( version_file ) ) {
        return load_json_file(version_file.generic_string(), _version);
    }

    return SUCCESS();
} // load_version_file

int _rsServerReport( rsComm_t* _comm, bytesBuf_t** _bbuf )
{
    if ( !_comm || !_bbuf ) {
        rodsLog( LOG_ERROR, "_rsServerReport: null comm or bbuf" );
        return SYS_INVALID_INPUT_PARAM;
    }

    ( *_bbuf ) = ( bytesBuf_t* ) malloc( sizeof( **_bbuf ) );
    if ( !( *_bbuf ) ) {
        rodsLog( LOG_ERROR, "_rsServerReport: failed to allocate _bbuf" );
        return SYS_MALLOC_ERR;
    }

    auto resc_svr = json::object();

    json version;
    irods::error ret = load_version_file( version );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }
    resc_svr["version"] = version;

    json host_system_information;
    ret = get_host_system_information( host_system_information );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }
    resc_svr["host_system_information"] = host_system_information;

    json svr_cfg;
    ret = convert_server_config( svr_cfg );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }
    resc_svr["server_config"] = svr_cfg;

    json host_ctrl;
    ret = convert_host_access_control( host_ctrl );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }
    resc_svr["host_access_control_config"] = host_ctrl;

    json irods_host;
    ret = convert_irods_host( irods_host );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }
    resc_svr["hosts_config"] = irods_host;

    json svc_acct;
    ret = convert_service_account( svc_acct );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }
    resc_svr["service_account_environment"] = svc_acct;

    json plugins;
    ret = irods::get_plugin_array( plugins );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }
    resc_svr["plugins"] = plugins;

    json resources;
    ret = get_resource_array( resources );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }
    resc_svr["resources"] = resources;

    json cfg_dir;
    ret = get_config_dir( cfg_dir );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }
    resc_svr["configuration_directory"] = cfg_dir;

    std::string svc_role;
    ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    const auto rs = resc_svr.dump(4);
    char* tmp_buf = new char[rs.length() + 1]{};
    std::strncpy(tmp_buf, rs.c_str(), rs.length());

    ( *_bbuf )->buf = tmp_buf;
    ( *_bbuf )->len = rs.length();

    return 0;
} // _rsServerReport
