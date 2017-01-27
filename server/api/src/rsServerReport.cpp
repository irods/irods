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

#include "jansson.h"

#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
namespace fs = boost::filesystem;

#include <sys/utsname.h>

int _rsServerReport(
    rsComm_t*    _comm,
    bytesBuf_t** _bbuf );

int rsServerReport(
    rsComm_t*    _comm,
    bytesBuf_t** _bbuf ) {

    // always execute this locally
    int status = _rsServerReport(
                     _comm,
                     _bbuf );
    if ( status < 0 ) {
        rodsLog(
            LOG_ERROR,
            "rsServerReport: rcServerReport failed, status = %d",
            status );
    }

    return status;

} // rsServerReport

json_t* make_federation_set(
    const std::vector< std::string >& _feds ) {
    if ( _feds.empty() ) {
        return nullptr;
    }

    json_t * _object = json_array();
    if ( !_object ) {
        THROW(SYS_MALLOC_ERR, "allocation of json object failed");
    }

    for ( const auto& federation : _feds ) {
        std::vector<std::string> zone_sid_vals;
        boost::split( zone_sid_vals, federation, boost::is_any_of( "-" ) );

        if ( zone_sid_vals.size() > 2 ) {
            rodsLog(
                LOG_ERROR,
                "multiple hyphens found in RemoteZoneSID [%s]",
                federation.c_str() );
            continue;
        }

        json_t* fed_obj = json_object();
        json_object_set_new( fed_obj, "zone_name",       json_string( zone_sid_vals[ 0 ].c_str() ) );
        json_object_set_new( fed_obj, "zone_key",        json_string( "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" ) );
        json_object_set_new( fed_obj, "negotiation_key", json_string( "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" ) );

        json_array_append_new( _object, fed_obj );

    } // for i

    return _object;

} // make_federation_set

irods::error sanitize_server_config_keys(
    json_t* _svr_cfg ) {
    if ( !_svr_cfg ) {
        return ERROR(
                   SYS_INVALID_INPUT_PARAM,
                   "null json object" );

    }

    // sanitize the top level keys
    json_object_set_new(
        _svr_cfg,
        "zone_key",
        json_string( "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" ) );

    json_object_set_new(
        _svr_cfg,
        "negotiation_key",
        json_string( "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" ) );

    json_object_set_new(
        _svr_cfg,
        "server_control_plane_key",
        json_string( "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" ) );

    // get the federation object
    json_t* fed_obj = json_object_get(
                          _svr_cfg,
                          "federation" );
    if ( !fed_obj ) {
        return SUCCESS();

    }

    // sanitize all federation keys
    size_t      idx = 0;
    json_t*     obj = 0;
    json_array_foreach( fed_obj, idx, obj ) {
        json_object_set_new(
            obj,
            "negotiation_key",
            json_string( "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" ) );
        json_object_set_new(
            obj,
            "zone_key",
            json_string( "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" ) );
    }

    return SUCCESS();

} // sanitize_server_config_keys

irods::error convert_server_config(
    json_t*& _svr_cfg ) {
    // =-=-=-=-=-=-=-
    // if json file exists, simply load that
    std::string svr_cfg;
    irods::error ret = irods::get_full_path_for_config_file(
                           "server_config.json",
                           svr_cfg );
    if ( !ret.ok() || !fs::exists( svr_cfg ) ) {
        return ERROR(SYS_CONFIG_FILE_ERR, boost::format("Could not find server config file at [%s]") % svr_cfg);
    }

    json_error_t error;
    _svr_cfg = json_load_file(
            svr_cfg.c_str(),
            0, &error );
    if ( !_svr_cfg ) {
        std::string msg( "failed to load file [" );
        msg += svr_cfg;
        msg += "] json error [";
        msg += error.text;
        msg += "]";
        return ERROR(
                -1,
                msg );

    }
    else {
        return sanitize_server_config_keys( _svr_cfg );

    }

} // convert_server_config



irods::error convert_host_access_control(
    json_t*& _host_ctrl ) {

    std::string cfg_file;
    irods::error ret = irods::get_full_path_for_config_file(
                           HOST_ACCESS_CONTROL_FILE,
                           cfg_file );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    json_error_t error;
    _host_ctrl = json_load_file(
                     cfg_file.c_str(),
                     0, &error );
    if ( !_host_ctrl ) {
        std::string msg( "failed to load file [" );
        msg += cfg_file;
        msg += "] json error [";
        msg += error.text;
        msg += "]";
        return ERROR(
                   -1,
                   msg );

    }

    return SUCCESS();

} // convert_host_access_control

irods::error convert_irods_host(
    json_t*& _irods_host ) {

    std::string cfg_file;
    irods::error ret = irods::get_full_path_for_config_file(
                           HOST_CONFIG_FILE,
                           cfg_file );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    json_error_t error;
    _irods_host = json_load_file(
                      cfg_file.c_str(),
                      0, &error );
    if ( !_irods_host ) {
        std::string msg( "failed to load file [" );
        msg += cfg_file;
        msg += "] json error [";
        msg += error.text;
        msg += "]";
        return ERROR(
                   -1,
                   msg );

    }

    return SUCCESS();

} // convert_irods_host

irods::error convert_service_account(
    json_t*& _svc_acct ) {
    std::string env_file_path;
    std::string session_file_path;
    irods::error ret = irods::get_json_environment_file(env_file_path, session_file_path);
    if (!ret.ok()) {
        return PASS(ret);
    }

    if (!fs::exists(env_file_path)) {
        return ERROR(SYS_CONFIG_FILE_ERR, boost::format("Could not find environment file at [%s]") % env_file_path);
    }

    json_error_t error;
    _svc_acct = json_load_file(env_file_path.c_str(), 0, &error);
    if ( !_svc_acct ) {
        return ERROR(-1, boost::format("failed to load [%s], json error [%s]") % env_file_path % error.text);
    } else {
        // sanitize the keys
        json_object_set_new(_svc_acct, "irods_server_control_plane_key", json_string( "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" ) );
        return SUCCESS();
    }
} // convert_service_account


irods::error get_uname_string(
    std::string& _str ) {

    struct utsname os_name;
    memset( &os_name, 0, sizeof( os_name ) );
    const int status = uname( &os_name );
    if ( status != 0 ) {
        return ERROR(
                   status,
                   "uname failed" );
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

irods::error get_user_name_string(std::string& user_name_string) {
    uid_t uid = getuid();
    passwd *pw = getpwuid(uid);
    if (pw==nullptr) {
        return ERROR(SYS_INTERNAL_NULL_INPUT_ERR, boost::format("getpwuid() returned null for uid [%d]") % uid);
    }
    user_name_string = pw->pw_name;
    return SUCCESS();
}

irods::error get_group_name_string(std::string& group_name_string) {
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

irods::error get_host_system_information(
    json_t*& _host_system_information ) {

    _host_system_information = json_object();
    if ( !_host_system_information ) {
        return ERROR(SYS_MALLOC_ERR, "json_object() failed" );
    }

    std::string user_name_string;
    irods::error ret = get_user_name_string(user_name_string);
    if (ret.ok()) {
        json_object_set_new( _host_system_information, "service_account_user_name", json_string( user_name_string.c_str() ) );
    } else {
        irods::log(ret);
        json_object_set_new( _host_system_information, "service_account_user_name", json_null());
    }

    std::string group_name_string;
    ret = get_group_name_string(group_name_string);
    if (ret.ok()) {
        json_object_set_new( _host_system_information, "service_account_group_name", json_string( group_name_string.c_str() ) );
    } else {
        irods::log(ret);
        json_object_set_new( _host_system_information, "service_account_group_name", json_null());
    }

    char hostname_buf[512];
    const int gethostname_ret = gethostname(hostname_buf, sizeof(hostname_buf));
    if (gethostname_ret != 0) {
        rodsLog(LOG_ERROR, "get_host_system_information: gethostname() failed [%d]", errno);
        json_object_set_new( _host_system_information, "hostname", json_null());
    } else {
        json_object_set_new( _host_system_information, "hostname", json_string(hostname_buf));
    }

    std::string uname_string;
    ret = get_uname_string( uname_string );
    if ( ret.ok() ) {
        json_object_set_new( _host_system_information, "uname", json_string( uname_string.c_str() ) );
    } else {
        irods::log( PASS( ret ) );
        json_object_set_new( _host_system_information, "uname", json_null());
    }

    std::vector<std::string> args;
    args.push_back( "os_distribution_name" );
    std::string os_distribution_name;
    ret = get_script_output_single_line( "python", "system_identification.py", args, os_distribution_name );
    if ( ret.ok() ) {
        json_object_set_new( _host_system_information, "os_distribution_name", json_string( os_distribution_name.c_str() ) );
    } else {
        irods::log( PASS( ret ) );
        json_object_set_new( _host_system_information, "os_distribution_name", json_null());
    }

    args.clear();
    args.push_back( "os_distribution_version" );
    std::string os_distribution_version;
    ret = get_script_output_single_line( "python", "system_identification.py", args, os_distribution_version );
    if (ret.ok()) {
        json_object_set_new( _host_system_information, "os_distribution_version", json_string( os_distribution_version.c_str() ) );
    } else {
        irods::log( PASS( ret ) );
        json_object_set_new( _host_system_information, "os_distribution_version", json_null());
    }

    return SUCCESS();

} // get_host_system_information

irods::error get_resource_array(
    json_t*& _resources ) {

    _resources = json_array();
    if ( !_resources ) {
        return ERROR(
                   SYS_MALLOC_ERR,
                   "json_array() failed" );
    }

    rodsEnv my_env;
    int status = getRodsEnv( &my_env );
    if ( status < 0 ) {
        return ERROR(
                   status,
                   "failed in getRodsEnv" );
    }

    const std::string local_host_name = my_env.rodsHost;

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

} // get_resource_array



irods::error get_file_contents(
    const std::string _fn,
    std::string&       _cont ) {

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



irods::error get_config_dir(
    json_t*& _cfg_dir ) {
    namespace fs = boost::filesystem;

    _cfg_dir = json_object();
    if ( !_cfg_dir ) {
        return ERROR(
                   SYS_MALLOC_ERR,
                   "json_object() failed" );
    }

    json_t* files = json_object();
    if ( !files ) {
        return ERROR(
                   SYS_MALLOC_ERR,
                   "json_object() failed" );
    }

    std::string cfg_file;
    irods::error ret = irods::get_full_path_for_config_file( irods::SERVER_CONFIG_FILE, cfg_file );

    fs::path p( cfg_file );
    std::string config_dir = p.parent_path().string();

    json_object_set_new( _cfg_dir, "path", json_string( config_dir.c_str() ) );

    for ( fs::directory_iterator itr( config_dir );
            itr != fs::directory_iterator();
            ++itr ) {
        if ( fs::is_regular_file( itr->path() ) ) {
            const fs::path& p = itr->path();
            const std::string name = p.string();

            if ( std::string::npos != name.find( irods::SERVER_CONFIG_FILE ) ||
                    std::string::npos != name.find( HOST_CONFIG_FILE ) ||
                    std::string::npos != name.find( HOST_ACCESS_CONTROL_FILE )
               ) {
                continue;
            }

            std::string contents;
            ret = get_file_contents( name, contents );

            if ( !ret.ok() ) {
                irods::log( PASS( ret ) );
                continue;
            }
            json_object_set_new(
                files,
                name.c_str(),
                json_string(contents.c_str()) );
        }

    } // for itr

    json_object_set_new( _cfg_dir, "files", files );

    return SUCCESS();

} // get_config_dir

irods::error load_version_file(
    json_t*& _version ) {
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
        json_error_t error;
        _version = json_load_file(
            version_file.string().c_str(),
                       0, &error );
        if ( !_version ) {
            std::string msg( "failed to load file [" );
            msg += version_file.string();
            msg += "] json error [";
            msg += error.text;
            msg += "]";
            return ERROR(-1, msg );
        }
        else {
            return SUCCESS();
        }
    }

    return SUCCESS();

} // load_version_file

int _rsServerReport(
    rsComm_t*    _comm,
    bytesBuf_t** _bbuf ) {

    if ( !_comm || !_bbuf ) {
        rodsLog(
            LOG_ERROR,
            "_rsServerReport: null comm or bbuf" );
        return SYS_INVALID_INPUT_PARAM;
    }

    ( *_bbuf ) = ( bytesBuf_t* ) malloc( sizeof( **_bbuf ) );
    if ( !( *_bbuf ) ) {
        rodsLog(
            LOG_ERROR,
            "_rsServerReport: failed to allocate _bbuf" );
        return SYS_MALLOC_ERR;

    }

    json_t* resc_svr = json_object();
    if ( !resc_svr ) {
        rodsLog(
            LOG_ERROR,
            "_rsServerReport: failed to allocate resc_svr" );
        return SYS_MALLOC_ERR;

    }

    json_t* version = 0;
    irods::error ret = load_version_file(
                           version );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }
    json_object_set_new( resc_svr, "version", version );

    json_t* host_system_information = 0;
    ret = get_host_system_information( host_system_information );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }
    json_object_set_new( resc_svr, "host_system_information", host_system_information );

    json_t* svr_cfg = 0;
    ret = convert_server_config( svr_cfg );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }
    json_object_set_new( resc_svr, "server_config", svr_cfg );

    json_t* host_ctrl = 0;
    ret = convert_host_access_control( host_ctrl );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }
    json_object_set_new( resc_svr, "host_access_control_config", host_ctrl );

    json_t* irods_host = 0;
    ret = convert_irods_host( irods_host );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }
    json_object_set_new( resc_svr, "hosts_config", irods_host );

    json_t* svc_acct = 0;
    ret = convert_service_account( svc_acct );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }
    json_object_set_new( resc_svr, "service_account_environment", svc_acct );

    json_t* plugins = 0;
    ret = irods::get_plugin_array( plugins );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }
    json_object_set_new( resc_svr, "plugins", plugins );

    json_t* resources = 0;
    ret = get_resource_array( resources );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }
    json_object_set_new( resc_svr, "resources", resources );

    json_t* cfg_dir = 0;
    ret = get_config_dir( cfg_dir );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }
    json_object_set_new( resc_svr, "configuration_directory", cfg_dir );

    std::string svc_role;
    ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    char* tmp_buf = json_dumps( resc_svr, JSON_INDENT( 4 ) );
    json_decref( resc_svr );

    ( *_bbuf )->buf = tmp_buf;
    ( *_bbuf )->len = strlen( tmp_buf );

    return 0;

} // _rsServerReport
