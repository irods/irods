
#include "rsGlobalExtern.hpp"
#include "rodsErrorTable.hpp"
#include "miscServerFunct.hpp"
#include "specificQuery.hpp"

#include "irods_log.hpp"
#include "zone_report.hpp"
#include "ies_client_hints.hpp"
#include "irods_resource_manager.hpp"
#include "irods_resource_backport.hpp"
#include "irods_server_properties.hpp"

#include <jansson.h>



int _rsIESClientHints(
    rsComm_t*    _comm,
    bytesBuf_t** _bbuf );

int rsIESClientHints(
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
#ifdef RODS_CAT
        status = _rsIESClientHints(
                     _comm,
                     _bbuf );
#else
        status = SYS_NO_RCAT_SERVER_ERR;
#endif
    }
    else {
        status = rcIESClientHints( rods_host->conn,
                                   _bbuf );
    }

    if ( status < 0 ) {
        rodsLog(
            LOG_ERROR,
            "rsIESClientHints: rcIESClientHints failed, status = %d",
            status );
    }

    return status;

} // rsIESClientHints


#ifdef RODS_CAT

irods::error get_strict_acls(
    rsComm_t*    _comm,
    std::string& _acls ) {
    if ( ! _comm ) {
        return ERROR(
                   SYS_INVALID_INPUT_PARAM,
                   "comm is null" );
    }

    irods::server_properties& props = irods::server_properties::getInstance();
    props.capture_if_needed();
    irods::error ret = props.get_property<std::string>(
                           irods::STRICT_ACL_KW,
                           _acls );

    return PASS( ret );

} // get_strict_acls



irods::error get_query_array(
    rsComm_t* _comm,
    json_t*&  _queries ) {
    if ( !_comm ) {
        return ERROR(
                   SYS_INVALID_INPUT_PARAM,
                   "comm is null" );
    }

    _queries = json_array();
    if ( !_queries ) {
        return ERROR(
                   SYS_MALLOC_ERR,
                   "allocation of json object failed" );
    }

    specificQueryInp_t spec_inp;
    memset( &spec_inp, 0, sizeof( specificQueryInp_t ) );

    spec_inp.maxRows = MAX_SQL_ROWS;
    spec_inp.continueInx = 0;
    spec_inp.sql = "ls";

    genQueryOut_t* gen_out = 0;
    int status = rsSpecificQuery(
                     _comm,
                     &spec_inp,
                     &gen_out );
    if ( status < 0 ) {
        return ERROR(
                   status,
                   "rsSpecificQuery for 'ls' failed" );
    }

    // first attribute is the alias of the specific query
    int   len    = gen_out->sqlResult[ 0 ].len;
    char* values = gen_out->sqlResult[ 0 ].value;

    for ( int i = 0 ;
            i < gen_out->rowCnt ;
            ++i ) {

        char* alias = &values[ len * i ];
        json_array_append( _queries, json_string( alias ) );

    } // for i

    freeGenQueryOut( &gen_out );

    return SUCCESS();

} // get_query_array



int _rsIESClientHints(
    rsComm_t*    _comm,
    bytesBuf_t** _bbuf ) {
    if ( ! _comm ) {
        return SYS_INVALID_INPUT_PARAM;

    }

    json_t* ies_hints = json_object();
    if ( !ies_hints ) {
        rodsLog(
            LOG_ERROR,
            "failed to allocate json_object" );
        return SYS_MALLOC_ERR;
    }

    std::string acls;
    irods::error ret = get_strict_acls(
                           _comm,
                           acls );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    json_object_set(
        ies_hints,
        "strict_acls",
        json_string( acls.c_str() ) );

    json_t* query_arr = 0;
    ret = get_query_array( _comm, query_arr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }
    json_object_set(
        ies_hints,
        "specific_queries",
        query_arr );

    char* tmp_buf = json_dumps( ies_hints, JSON_INDENT( 4 ) );

    // *SHOULD* free All The Things...
    json_decref( ies_hints );

    ( *_bbuf ) = ( bytesBuf_t* ) malloc( sizeof( bytesBuf_t ) );
    if ( !( *_bbuf ) ) {
        rodsLog(
            LOG_ERROR,
            "_rsIESClientHints: failed to allocate _bbuf" );
        return SYS_MALLOC_ERR;

    }

    ( *_bbuf )->buf = tmp_buf;
    ( *_bbuf )->len = strlen( tmp_buf );

    return 0;

} // _rsIESClientHints

#endif // RODS_CAT

