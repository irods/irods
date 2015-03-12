/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See genQuery.h for a description of this API call.*/

#include "reFuncDefs.hpp"
#include "genQuery.hpp"
#include "icatHighLevelRoutines.hpp"
#include "miscUtil.hpp"
#include "cache.hpp"
#include "rsGlobalExtern.hpp"
#include "irods_server_properties.hpp"

#include "boost/format.hpp"
#include <string>

static
irods::error strip_new_query_terms(
    genQueryInp_t* _inp ) {
    // =-=-=-=-=-=-=-
    // cache pointers to the incoming inxIvalPair
    inxIvalPair_t tmp;
    tmp.len   = _inp->selectInp.len;
    tmp.inx   = _inp->selectInp.inx;
    tmp.value = _inp->selectInp.value;

    // =-=-=-=-=-=-=-
    // zero out the selectInp to copy
    // fresh community indices and values
    bzero( &_inp->selectInp, sizeof( _inp->selectInp ) );

    // =-=-=-=-=-=-=-
    // iterate over the tmp and only copy community values
    for ( int i = 0; i < tmp.len; ++i ) {
        if ( tmp.inx[ i ] == COL_R_RESC_CHILDREN ||
                tmp.inx[ i ] == COL_R_RESC_CONTEXT  ||
                tmp.inx[ i ] == COL_R_RESC_PARENT   ||
                tmp.inx[ i ] == COL_R_RESC_OBJCOUNT ||
                tmp.inx[ i ] == COL_D_RESC_HIER ) {
            continue;
        }
        else {
            addInxIval( &_inp->selectInp, tmp.inx[ i ], tmp.value[ i ] );
        }

    } // for i

    return SUCCESS();

} // strip_new_query_terms


static
irods::error strip_resc_grp_name_from_query_inp( genQueryInp_t* _inp, int& _pos ) {

    const int COL_D_RESC_GROUP_NAME  = 408;

    // sanity check
    if ( !_inp ) {
        return CODE( SYS_INTERNAL_NULL_INPUT_ERR );
    }

    // =-=-=-=-=-=-=-
    // cache pointers to the incoming inxIvalPair
    inxIvalPair_t tmp;
    tmp.len   = _inp->selectInp.len;
    tmp.inx   = _inp->selectInp.inx;
    tmp.value = _inp->selectInp.value;

    // =-=-=-=-=-=-=-
    // zero out the selectInp to copy
    // fresh indices and values
    bzero( &_inp->selectInp, sizeof( _inp->selectInp ) );

    // =-=-=-=-=-=-=-
    // iterate over tmp and replace resource group with resource name
    for ( int i = 0; i < tmp.len; ++i ) {
        if ( tmp.inx[i] == COL_D_RESC_GROUP_NAME ) {
            addInxIval( &_inp->selectInp, COL_D_RESC_NAME, tmp.value[i] );
            _pos = i;
        }
        else {
            addInxIval( &_inp->selectInp, tmp.inx[i], tmp.value[i] );
        }
    } // for i

    // cleanup
    if ( tmp.inx ) { free( tmp.inx ); }
    if ( tmp.value ) { free( tmp.value ); }

    return SUCCESS();

} // strip_resc_grp_name_from_query_inp


static
irods::error add_resc_grp_name_to_query_out( genQueryOut_t *_out, int& _pos ) {

    const int COL_D_RESC_GROUP_NAME  = 408;

    // =-=-=-=-=-=-=-
    // Sanity checks
    if ( !_out ) {
        return CODE( SYS_INTERNAL_NULL_INPUT_ERR );
    }

    if ( _pos < 0 || _pos > MAX_SQL_ATTR - 1 ) {
        return CODE( SYS_INVALID_INPUT_PARAM );
    }

    sqlResult_t *sqlResult = &_out->sqlResult[_pos];
    if ( !sqlResult || sqlResult->attriInx != COL_D_RESC_NAME ) {
        return CODE( SYS_INTERNAL_ERR );
    }

    // =-=-=-=-=-=-=-
    // Swap attribute indices back
    sqlResult->attriInx = COL_D_RESC_GROUP_NAME;

    return SUCCESS();

} // add_resc_grp_name_to_query_out


static
irods::error proc_query_terms_for_community_server(
    const std::string& _zone_hint,
    genQueryInp_t*     _inp ) {
    bool        done     = false;
    zoneInfo_t* tmp_zone = ZoneInfoHead;
    // =-=-=-=-=-=-=-
    // if the zone hint starts with a / we
    // will need to pull out just the zone
    std::string zone_hint = _zone_hint;
    if ( _zone_hint[0] == '/' ) {
        size_t pos = _zone_hint.find( "/", 1 );
        if ( std::string::npos != pos ) {
            zone_hint = _zone_hint.substr( 1, pos - 1 );
        }
        else {
            zone_hint = _zone_hint.substr( 1 );
        }

    }
    else {
        return SUCCESS();
    }

    // =-=-=-=-=-=-=-
    // grind through the zones and find the match to the kw
    while ( !done && tmp_zone ) {
        if ( zone_hint == tmp_zone->zoneName               &&
                tmp_zone->masterServerHost->conn              &&
                tmp_zone->masterServerHost->conn->svrVersion &&
                tmp_zone->masterServerHost->conn->svrVersion->cookie < 301 ) {
            return strip_new_query_terms( _inp );

        }
        else {
            tmp_zone = tmp_zone->next;

        }
    }

    return SUCCESS();

} // proc_query_terms_for_community_server

/* can be used for debug: */
/* extern int printGenQI( genQueryInp_t *genQueryInp); */
;
int
rsGenQuery( rsComm_t *rsComm, genQueryInp_t *genQueryInp,
            genQueryOut_t **genQueryOut ) {
    rodsServerHost_t *rodsServerHost;
    int status;
    char *zoneHint;
    zoneHint = getZoneHintForGenQuery( genQueryInp );

    std::string zone_hint_str;
    if ( zoneHint ) {
        zone_hint_str = zoneHint;
    }

    status = getAndConnRcatHost( rsComm, SLAVE_RCAT, ( const char* )zoneHint,
                                 &rodsServerHost );

    if ( status < 0 ) {
        return status;
    }

    // =-=-=-=-=-=-=-
    // handle connections with community iRODS
    if ( !zone_hint_str.empty() ) {
        irods::error ret = proc_query_terms_for_community_server( zone_hint_str, genQueryInp );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
        }
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
#ifdef RODS_CAT
        status = _rsGenQuery( rsComm, genQueryInp, genQueryOut );
#else
        rodsLog( LOG_NOTICE,
                 "rsGenQuery error. RCAT is not configured on this host" );
        return SYS_NO_RCAT_SERVER_ERR;
#endif
    }
    else {
        // =-=-=-=-=-=-=-
        // strip disable strict acl flag if the agent conn flag is missing
        char* dis_kw = getValByKey( &genQueryInp->condInput, DISABLE_STRICT_ACL_KW );
        if ( dis_kw ) {
            irods::server_properties& props = irods::server_properties::getInstance();
            props.capture_if_needed();

            std::string svr_sid;
            irods::error err = props.get_property< std::string >( irods::AGENT_CONN_KW, svr_sid );
            if ( !err.ok() ) {
                rmKeyVal( &genQueryInp->condInput, DISABLE_STRICT_ACL_KW );

            }

        } // if dis_kw

        status = rcGenQuery( rodsServerHost->conn,
                             genQueryInp, genQueryOut );
    }
    if ( status < 0  && status != CAT_NO_ROWS_FOUND ) {
        std::string prefix = ( rodsServerHost->localFlag == LOCAL_HOST ) ? "_rs" : "rc";
        rodsLog( LOG_NOTICE,
                 "rsGenQuery: %sGenQuery failed, status = %d", prefix.c_str(), status );
    }
    return status;
}

#ifdef RODS_CAT
int
_rsGenQuery( rsComm_t *rsComm, genQueryInp_t *genQueryInp,
             genQueryOut_t **genQueryOut ) {
    int status;
    int resc_grp_attr_pos = -1;

    static int ruleExecuted = 0;
    ruleExecInfo_t rei;


    static int PrePostProcForGenQueryFlag = -2;
    int i, argc;
    ruleExecInfo_t rei2;
    const char *args[MAX_NUM_OF_ARGS_IN_ACTION];

    // =-=-=-=-=-=-=-
    // handle queries from older clients
    std::string client_rel_version = rsComm->cliVersion.relVersion;
    std::string local_rel_version = RODS_REL_VERSION;
    if ( client_rel_version != local_rel_version ) {	// skip if version strings match
        irods::error err = strip_resc_grp_name_from_query_inp( genQueryInp, resc_grp_attr_pos );
        if ( !err.ok() ) {
            irods::log( PASS( err ) );
        }
    }

    if ( PrePostProcForGenQueryFlag < 0 ) {
        if ( getenv( "PREPOSTPROCFORGENQUERYFLAG" ) != NULL ) {
            PrePostProcForGenQueryFlag = 1;
        }
        else {
            PrePostProcForGenQueryFlag = 0;
        }
    }

    memset( ( char* )&rei2, 0, sizeof( ruleExecInfo_t ) );
    rei2.rsComm = rsComm;

    /*  printGenQI(genQueryInp);  for debug */

    *genQueryOut = ( genQueryOut_t* )malloc( sizeof( genQueryOut_t ) );
    memset( ( char * )*genQueryOut, 0, sizeof( genQueryOut_t ) );

    if ( ruleExecuted == 0 ) {
        memset( ( char* )&rei, 0, sizeof( rei ) );
        /* Include the user info for possible use by the rule.  Note
        that when this is called (as the agent is initializing),
        this user info is not confirmed yet.  For password
        authentication though, the agent will soon exit if this
        is not valid.  But for GSI, the user information may not
        be present and/or may be changed when the authentication
        completes, so it may not be safe to use this in a GSI
        enabled environment.  This addition of user information
        was requested by ARCS/IVEC (Sean Fleming) to avoid a
        local patch.
        */
        rei.uoic = &rsComm->clientUser;
        rei.uoip = &rsComm->proxyUser;

        if ( getRuleEngineStatus() == UNINITIALIZED ) {
            /* Skip the call to run acAclPolicy if the Rule Engine
               hasn't been initialized yet, which happens for a couple
               initial queries made by the agent when starting up.  The
               new RE logs these types of errors and so this avoids that.
            */
            status = -1;
        }
        else {
            status = applyRule( "acAclPolicy", NULL, &rei, NO_SAVE_REI );
        }
        if ( status == 0 ) {
            ruleExecuted = 1; /* No need to retry next time since it
                             succeeded.  Since this is called at
                             startup, the Rule Engine may not be
                             initialized yet, in which case the
                             default setting is fine and we should
                             retry next time. */
        }
    }

    // =-=-=-=-=-=-=-
    // verify that we are running a query for another agent connection
    irods::server_properties& props = irods::server_properties::getInstance();
    props.capture_if_needed();

    std::string svr_sid;
    irods::error err = props.get_property< std::string >( irods::AGENT_CONN_KW, svr_sid );
    bool agent_conn_flg = err.ok();

    // =-=-=-=-=-=-=-
    // detect if a request for disable of strict acls is made
    int acl_val = -1;
    char* dis_kw = getValByKey( &genQueryInp->condInput, DISABLE_STRICT_ACL_KW );
    if ( agent_conn_flg && dis_kw ) {
        acl_val = 0;
    }

    // =-=-=-=-=-=-=-
    // cache the old acl value for reuse later if necessary
    int old_acl_val =  chlGenQueryAccessControlSetup(
                           rsComm->clientUser.userName,
                           rsComm->clientUser.rodsZone,
                           rsComm->clientAddr,
                           rsComm->clientUser.authInfo.authFlag,
                           acl_val );

    if ( PrePostProcForGenQueryFlag == 1 ) {
        std::string arg = str( boost::format( "%ld" ) % ( ( long )genQueryInp ) );
        args[0] = arg.c_str();
        argc = 1;
        i =  applyRuleArg( "acPreProcForGenQuery", args, argc, &rei2, NO_SAVE_REI );
        if ( i < 0 ) {
            rodsLog( LOG_ERROR,
                     "rsGenQuery:acPreProcForGenQuery error,stat=%d", i );
            if ( i != NO_MICROSERVICE_FOUND_ERR ) {
                return i;
            }
        }
    }
    /**  June 1 2009 for pre-post processing rule hooks **/

    status = chlGenQuery( *genQueryInp, *genQueryOut );

    // =-=-=-=-=-=-=-
    // if a disable was requested, repave with old value immediately
    if ( agent_conn_flg && dis_kw ) {
        chlGenQueryAccessControlSetup(
            rsComm->clientUser.userName,
            rsComm->clientUser.rodsZone,
            rsComm->clientAddr,
            rsComm->clientUser.authInfo.authFlag,
            old_acl_val );
    }

    /**  June 1 2009 for pre-post processing rule hooks **/
    if ( PrePostProcForGenQueryFlag == 1 ) {
        std::string in_string = str( boost::format( "%ld" ) % ( ( long )genQueryInp ) );
        std::string out_string = str( boost::format( "%ld" ) % ( ( long )genQueryOut ) );
        std::string status_string = str( boost::format( "%d" ) % ( ( long )status ) );
        args[0] = in_string.c_str();
        args[1] = out_string.c_str();
        args[2] = status_string.c_str();
        argc = 3;
        i =  applyRuleArg( "acPostProcForGenQuery", args, argc, &rei2, NO_SAVE_REI );
        if ( i < 0 ) {
            rodsLog( LOG_ERROR,
                     "rsGenQuery:acPostProcForGenQuery error,stat=%d", i );
            if ( i != NO_MICROSERVICE_FOUND_ERR ) {
                return i;
            }
        }
    }
    /**  June 1 2009 for pre-post processing rule hooks **/

    // =-=-=-=-=-=-=-
    // handle queries from older clients
    if ( status >= 0 && resc_grp_attr_pos >= 0 ) {
        irods::error err = add_resc_grp_name_to_query_out( *genQueryOut, resc_grp_attr_pos );
        if ( !err.ok() ) {
            irods::log( PASS( err ) );
        }
    }

    if ( status < 0 ) {
        clearGenQueryOut( *genQueryOut );
        free( *genQueryOut );
        *genQueryOut = NULL;
        if ( status != CAT_NO_ROWS_FOUND ) {
            rodsLog( LOG_NOTICE,
                     "_rsGenQuery: genQuery status = %d", status );
        }
        return status;
    }
    return status;
}
#endif
