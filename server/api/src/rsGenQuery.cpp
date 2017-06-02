/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See genQuery.h for a description of this API call.*/

#include "rcMisc.h"
#include "genQuery.h"
#include "icatHighLevelRoutines.hpp"
#include "miscUtil.h"
#include "rsGenQuery.hpp"
#include "rsGlobalExtern.hpp"
#include "miscServerFunct.hpp"
#include "irods_server_properties.hpp"
#include "irods_lexical_cast.hpp"

#include "boost/format.hpp"
#include <boost/tokenizer.hpp>
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
                tmp.inx[ i ] == COL_R_RESC_PARENT_CONTEXT   ||
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
irods::error get_resc_id_cond_for_hier_cond(
        const char* _cond,
       std::string& _new_cond ) {
    std::string in_cond( _cond );
    std::string::size_type p0 = in_cond.find_first_of("'");
    std::string::size_type p1 = in_cond.find_last_of("'");
    if (p1==p0) {
        return ERROR(
            SYS_INVALID_INPUT_PARAM,
            _cond );
    }

    std::string hier = in_cond.substr(p0+1, p1-p0-1);

    rodsLong_t id = 0;
    irods::error ret = resc_mgr.hier_to_leaf_id(hier,id);
    if(!ret.ok()) {
        return PASS(ret);
    }

    std::stringstream idstr; idstr << id;
    _new_cond = "='"+idstr.str()+"'";

    return SUCCESS();

} // get_resc_id_cond_for_hier_cond

static
irods::error strip_resc_hier_name_from_query_inp( genQueryInp_t* _inp, int& _pos ) {
    // sanity check
    if ( !_inp ) {
        return CODE( SYS_INTERNAL_NULL_INPUT_ERR );
    }

    // =-=-=-=-=-=-=-
    // cache pointers to the incoming inxIvalPair
    inxIvalPair_t tmpI;
    tmpI.len   = _inp->selectInp.len;
    tmpI.inx   = _inp->selectInp.inx;
    tmpI.value = _inp->selectInp.value;

    // =-=-=-=-=-=-=-
    // zero out the selectInp to copy
    // fresh indices and values
    bzero( &_inp->selectInp, sizeof( _inp->selectInp ) );

    // =-=-=-=-=-=-=-
    // iterate over tmp and replace resource group with resource name
    for ( int i = 0; i < tmpI.len; ++i ) {
        if ( tmpI.inx[i] == COL_D_RESC_HIER ) {
            addInxIval( &_inp->selectInp, COL_D_RESC_ID, tmpI.value[i] );
            _pos = i;
        }
        else {
            addInxIval( &_inp->selectInp, tmpI.inx[i], tmpI.value[i] );
        }
    } // for i

    // cleanup
    if ( tmpI.inx ) { free( tmpI.inx ); }
    if ( tmpI.value ) { free( tmpI.value ); }


    // =-=-=-=-=-=-=-
    // cache pointers to the incoming inxIvalPair
    inxValPair_t tmpV;
    tmpV.len   = _inp->sqlCondInp.len;
    tmpV.inx   = _inp->sqlCondInp.inx;
    tmpV.value = _inp->sqlCondInp.value;

    // =-=-=-=-=-=-=-
    // zero out the selectInp to copy
    // fresh indices and values
    bzero( &_inp->sqlCondInp, sizeof( _inp->selectInp ) );

    // =-=-=-=-=-=-=-
    // iterate over tmp and replace resource group with resource name
    for ( int i = 0; i < tmpV.len; ++i ) {
        if ( tmpV.inx[i] == COL_D_RESC_HIER ) {
            std::string new_cond = "='0'";
            irods::error ret = get_resc_id_cond_for_hier_cond(
                                   tmpV.value[i],
                                   new_cond);
            if(!ret.ok()) {
                irods::log(PASS(ret));
            }

            addInxVal( &_inp->sqlCondInp, COL_D_RESC_ID, new_cond.c_str() );
            _pos = i;
        }
        else {
            addInxVal( &_inp->sqlCondInp, tmpV.inx[i], tmpV.value[i] );
        }
    } // for i

    // cleanup
    if ( tmpV.inx ) { free( tmpV.inx ); }
    if ( tmpV.value ) { free( tmpV.value ); }

    return SUCCESS();

} // strip_resc_hier_name_from_query_inp


static
irods::error add_resc_hier_name_to_query_out( genQueryOut_t *_out, int& _pos ) {
    // =-=-=-=-=-=-=-
    // Sanity checks
    if ( !_out ) {
        return CODE( SYS_INTERNAL_NULL_INPUT_ERR );
    }

    if ( _pos < 0 || _pos > MAX_SQL_ATTR - 1 ) {
        return CODE( SYS_INVALID_INPUT_PARAM );
    }

    sqlResult_t *sqlResult = &_out->sqlResult[_pos];
    if ( !sqlResult || sqlResult->attriInx != COL_D_RESC_ID ) {
        return CODE( SYS_INTERNAL_ERR );
    }

    // =-=-=-=-=-=-=-
    // Swap attribute indices back
    sqlResult->attriInx = COL_D_RESC_HIER;

    // =-=-=-=-=-=-=-
    // cache hier strings for leaf id
    size_t max_len = 0;
    std::vector<std::string> resc_hiers(_out->rowCnt);
    for ( int i = 0; i < _out->rowCnt; ++i ) {
        char* leaf_id_str = &sqlResult->value[i*sqlResult->len];

        rodsLong_t leaf_id = 0;
        irods::error ret = irods::lexical_cast<rodsLong_t>(
                               leaf_id_str,
                               leaf_id);
        if(!ret.ok()) {
            irods::log(PASS(ret));
            continue;
        }

        std::string hier;
        ret = resc_mgr.leaf_id_to_hier( leaf_id, resc_hiers[i] );
        if(!ret.ok()) {
            irods::log(PASS(ret));
            continue;
        }

        if(resc_hiers[i].size() > max_len ) {
            max_len = resc_hiers[i].size();
        }

    } // for i

    free( sqlResult->value );

    // =-=-=-=-=-=-=-
    // craft new result string with the hiers
    sqlResult->len = max_len+1;
    sqlResult->value = (char*)malloc(sqlResult->len*_out->rowCnt);
    for( std::size_t i = 0; i < resc_hiers.size(); ++i ) {
        snprintf(
            &sqlResult->value[i*sqlResult->len],
            sqlResult->len,
            "%s",
            resc_hiers[i].c_str() );
    }

    return SUCCESS();

} // add_resc_hier_name_to_query_out


static
irods::error proc_query_terms_for_community_server(
        const std::string& _zone_hint,
        genQueryInp_t*     _inp ){
    std::string zone_name;
    zoneInfo_t* tmp_zone = ZoneInfoHead;

    // =-=-=-=-=-=-=-
    // extract zone name from zone hint
    boost::char_separator<char> sep("/");
    boost::tokenizer<boost::char_separator<char> > tokens(_zone_hint, sep);
    if (tokens.begin() != tokens.end()) {
        zone_name = *tokens.begin();
    } else {
        return ERROR(SYS_INVALID_ZONE_NAME, "No zone name parsed from zone hint");
    }

    // =-=-=-=-=-=-=-
    // grind through the zones and find the match to the kw
    while (tmp_zone) {
        if ( zone_name == tmp_zone->zoneName               &&
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

#if 0 // debug code
int
print_gen_query_input(
    genQueryInp_t *genQueryInp ) {
    if(!genQueryInp) return 0;

    int i = 0, len = 0;
    int *ip1 = 0, *ip2 = 0;
    char *cp = 0;
    char **cpp = 0;

    printf( "maxRows=%d\n", genQueryInp->maxRows );

    len = genQueryInp->selectInp.len;
    printf( "sel len=%d\n", len );

    ip1 = genQueryInp->selectInp.inx;
    ip2 = genQueryInp->selectInp.value;
    if( ip1 && ip2 ) {
            for ( i = 0; i < len; i++ ) {
                printf( "sel inx [%d]=%d\n", i, *ip1 );
                printf( "sel val [%d]=%d\n", i, *ip2 );
                ip1++;
                ip2++;
            }
    }

    len = genQueryInp->sqlCondInp.len;
    printf( "sqlCond len=%d\n", len );
    ip1 = genQueryInp->sqlCondInp.inx;
    cpp = genQueryInp->sqlCondInp.value;
    if(cpp) {
        cp = *cpp;
    }
    if( ip1 && cp ) {
            for ( i = 0; i < len; i++ ) {
                printf( "sel inx [%d]=%d\n", i, *ip1 );
                printf( "sel val [%d]=:%s:\n", i, cp );
                ip1++;
                cpp++;
                cp = *cpp;
            }
    }

    return 0;
}
#endif

/* can be used for debug: */
/* extern int printGenQI( genQueryInp_t *genQueryInp); */

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

        // clean up path separator(s) and trailing '
        if('/' == zone_hint_str[0]) {
            zone_hint_str = zone_hint_str.substr(1);

            std::string::size_type pos = zone_hint_str.find_first_of("/");
            if(std::string::npos != pos ) {
                zone_hint_str = zone_hint_str.substr(0,pos);
            }
        }
        if('\'' == zone_hint_str[zone_hint_str.size()-1]) {
            zone_hint_str = zone_hint_str.substr(0,zone_hint_str.size()-1);
        }

    }

    status = getAndConnRcatHost(
                 rsComm,
                 SLAVE_RCAT,
                 zone_hint_str.c_str(),
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
        std::string svc_role;
        irods::error ret = get_catalog_service_role(svc_role);
        if(!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }

        if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            status = _rsGenQuery( rsComm, genQueryInp, genQueryOut );
        } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
            rodsLog( LOG_NOTICE,
                     "rsGenQuery error. RCAT is not configured on this host" );
            return SYS_NO_RCAT_SERVER_ERR;
        } else {
            rodsLog(
                LOG_ERROR,
                "role not supported [%s]",
                svc_role.c_str() );
            status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
        }
    }
    else {
        // =-=-=-=-=-=-=-
        // strip disable strict acl flag if the agent conn flag is missing
        char* dis_kw = getValByKey( &genQueryInp->condInput, DISABLE_STRICT_ACL_KW );
        if ( dis_kw ) {
            try {
                irods::get_server_property<const std::string>(irods::AGENT_CONN_KW);
            } catch ( const irods::exception& ) {
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

int
_rsGenQuery( rsComm_t *rsComm, genQueryInp_t *genQueryInp,
             genQueryOut_t **genQueryOut ) {
    int status;
    int resc_grp_attr_pos = -1;
    int resc_hier_attr_pos = -1;

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

    irods::error err = strip_resc_hier_name_from_query_inp( genQueryInp, resc_hier_attr_pos );
    if ( !err.ok() ) {
        irods::log( PASS( err ) );
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

        status = applyRule( "acAclPolicy", NULL, &rei, NO_SAVE_REI );
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
    bool agent_conn_flag = true;
    try {
        irods::get_server_property<const std::string>(irods::AGENT_CONN_KW);
    } catch ( const irods::exception& ) {
        agent_conn_flag = false;
    }

    // =-=-=-=-=-=-=-
    // detect if a request for disable of strict acls is made
    int acl_val = -1;
    char* dis_kw = getValByKey( &genQueryInp->condInput, DISABLE_STRICT_ACL_KW );
    if ( agent_conn_flag && dis_kw ) {
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
    if ( agent_conn_flag && dis_kw ) {
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

    if ( status >= 0 && resc_hier_attr_pos >= 0 ) {
        // restore COL_D_RESC_HIER in select index
        genQueryInp->selectInp.inx[resc_hier_attr_pos] = COL_D_RESC_HIER;

        // replace resc ids with resc hier strings in output
        irods::error err = add_resc_hier_name_to_query_out( *genQueryOut, resc_hier_attr_pos );
        if ( !err.ok() ) {
            irods::log( PASS( err ) );
        }
    }

    if ( status < 0 ) {
        if ( status != CAT_NO_ROWS_FOUND ) {
            rodsLog( LOG_NOTICE,
                     "_rsGenQuery: genQuery status = %d", status );
        }
        return status;
    }
    return status;
}
