#include "rcMisc.h"
#include "rsGlobalExtern.hpp"
#include "rodsErrorTable.h"
#include "rsClientHints.hpp"

#include "irods_server_properties.hpp"
#include "irods_log.hpp"
#include "irods_plugin_name_generator.hpp"
#include "irods_resource_manager.hpp"
#include "irods_get_full_path_for_config_file.hpp"
#include "irods_report_plugins_in_json.hpp"

#include "client_hints.h"
#include "ies_client_hints.h"
#include "rsIESClientHints.hpp"

#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>

#include <sys/utsname.h>

#include "json.hpp"

using json = nlohmann::json;

const std::string HOST_ACCESS_CONTROL_FILE( "HostAccessControl" );

int _rsClientHints( rsComm_t* _comm, bytesBuf_t** _bbuf );

int rsClientHints( rsComm_t* _comm, bytesBuf_t** _bbuf )
{
    // always execute this locally
    int status = _rsClientHints(
                     _comm,
                     _bbuf );
    if ( status < 0 ) {
        rodsLog(
            LOG_ERROR,
            "rsClientHints: rcClientHints failed, status = %d",
            status );
    }

    return status;
} // rsClientHints

irods::error get_hash_and_policy(
    rsComm_t* _comm,
    std::string& _hash,
    std::string& _policy )
{
    if ( !_comm ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "comm is null" );
    }

    try {
        _hash = irods::get_server_property<const std::string>(irods::CFG_DEFAULT_HASH_SCHEME_KW);
    } catch ( const irods::exception& ) {
        _hash = "SHA256";
    }

    try {
        _policy = irods::get_server_property<const std::string>(irods::CFG_MATCH_HASH_POLICY_KW);
    } catch ( const irods::exception& ) {
        _policy = "compatible";
    }

    return SUCCESS();
} // get_hash_and_policy

int _rsClientHints( rsComm_t*    _comm, bytesBuf_t** _bbuf )
{
    if ( !_comm || !_bbuf ) {
        rodsLog( LOG_ERROR, "_rsServerReport: null comm or bbuf" );
        return SYS_INVALID_INPUT_PARAM;
    }

    ( *_bbuf ) = ( bytesBuf_t* ) malloc( sizeof( bytesBuf_t ) );
    if ( !( *_bbuf ) ) {
        rodsLog( LOG_ERROR, "_rsClientHints: failed to allocate _bbuf" );
        return SYS_MALLOC_ERR;
    }

    bytesBuf_t* ies_buf = 0;
    int status = rsIESClientHints( _comm, &ies_buf );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "_rsClientHints: rsIESClientHints failed %d", status );
        return status;
    }

    json client_hints;

    try {
        client_hints = json::parse(std::string(static_cast<const char*>(ies_buf->buf), ies_buf->len));
        freeBBuf( ies_buf );
    }
    catch (const json::parse_error& e) {
        rodsLog( LOG_ERROR, "_rsClientHints - json::parse failed [%s]", e.what());
        return ACTION_FAILED_ERR;
    }

    std::string hash, hash_policy;
    irods::error ret = get_hash_and_policy( _comm, hash, hash_policy );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    client_hints["hash_scheme"] = hash;
    client_hints["match_hash_policy"] = hash_policy;

    json plugins;
    ret = irods::get_plugin_array(plugins);
    if (!ret.ok()) {
        irods::log(PASS(ret));
    }
    client_hints["plugins"] = plugins;

    // List rules
    ruleExecInfo_t rei;
    memset( ( char*) &rei, 0, sizeof( ruleExecInfo_t ) );
    rei.rsComm  = _comm;
    rei.uoic    = &_comm->clientUser;
    rei.uoip    = &_comm->proxyUser;
    rei.uoio    = nullptr;
    rei.coi     = nullptr;

    irods::rule_engine_context_manager<
        irods::unit,
        ruleExecInfo_t*,
        irods::DONT_AUDIT_RULE > re_ctx_mgr(
                                irods::re_plugin_globals->global_re_mgr,
                                &rei );

    std::vector< std::string > rule_vec;
    ret = re_ctx_mgr.list_rules( rule_vec );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    auto rules = json::array();
    for ( const auto& rule : rule_vec ) {
        rules.push_back(rule);
    }
    client_hints["rules"] = rules;

    const auto ch = client_hints.dump(4);
    char* tmp_buf = new char[ch.length() + 1]{};
    std::strncpy(tmp_buf, ch.c_str(), ch.length());

    ( *_bbuf )->buf = tmp_buf;
    ( *_bbuf )->len = ch.length();

    return 0;
} // _rsClientHints

