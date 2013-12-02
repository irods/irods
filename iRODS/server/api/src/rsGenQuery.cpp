/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See genQuery.h for a description of this API call.*/

#include "genQuery.hpp"
#include "icatHighLevelRoutines.hpp"
#include "miscUtil.hpp"
#include "cache.hpp"
#include "rsGlobalExtern.hpp"

static
eirods::error strip_eirods_query_terms( 
    genQueryInp_t* _inp ) {
    // =-=-=-=-=-=-=-
    // cache pointers to the incoming inxIvalPair
    inxIvalPair_t tmp;
    tmp.len   = _inp->selectInp.len;
    tmp.inx   = _inp->selectInp.inx;
    tmp.value = _inp->selectInp.value;
    
    // =-=-=-=-=-=-=-
    // zero out the selectInp to copy
    // fresh non-eirods indicies and values
    bzero( &_inp->selectInp, sizeof( _inp->selectInp ) );
     
    // =-=-=-=-=-=-=-
    // iterate over the tmp and copy non eirods values
    for( int i = 0; i < tmp.len; ++i ) {
        if( tmp.inx[ i ] == COL_R_RESC_CHILDREN ||
            tmp.inx[ i ] == COL_R_RESC_CONTEXT  ||
            tmp.inx[ i ] == COL_R_RESC_PARENT   ||
            tmp.inx[ i ] == COL_R_RESC_OBJCOUNT ||
            tmp.inx[ i ] == COL_D_RESC_HIER ) {
            continue;
        } else {
            addInxIval( &_inp->selectInp, tmp.inx[ i ], tmp.value[ i ] );
        }

    } // for i

    return SUCCESS();

} // strip_eirods_query_terms

static
eirods::error proc_query_terms_for_non_eirods_server( 
    const std::string& _zone_hint,
    genQueryInp_t*     _inp ) {
    bool        done     = false;
    zoneInfo_t* tmp_zone = ZoneInfoHead;
    // =-=-=-=-=-=-=-
    // if the zone hint starts with a / we
    // will need to pull out just the zone
    std::string zone_hint = _zone_hint;
    if( _zone_hint[0] == '/' ) {
        size_t pos = _zone_hint.find( "/", 1 );
        if( std::string::npos != pos ) {
            zone_hint = _zone_hint.substr( 1, pos-1 );
        } else {
            return ERROR( 
                       SYS_INVALID_INPUT_PARAM, 
                       "error finding zone hint" );
        }
    }

    // =-=-=-=-=-=-=-
    // grind through the zones and find the match to the kw
    while( !done && tmp_zone ) {
        if( zone_hint == tmp_zone->zoneName               &&
            tmp_zone->masterServerHost->conn              &&
            tmp_zone->masterServerHost->conn->svrVersion &&
            tmp_zone->masterServerHost->conn->svrVersion->cookie < 301 ) {
            return strip_eirods_query_terms( _inp );

        } else {
            tmp_zone = tmp_zone->next;

        }
    }

    return SUCCESS();

} // proc_query_terms_for_non_eirods_server

/* can be used for debug: */
/* extern int printGenQI( genQueryInp_t *genQueryInp); */
;
int
rsGenQuery (rsComm_t *rsComm, genQueryInp_t *genQueryInp, 
genQueryOut_t **genQueryOut)
{
    rodsServerHost_t *rodsServerHost;
    int status;
    char *zoneHint;
    zoneHint = getZoneHintForGenQuery (genQueryInp);
    
    std::string zone_hint_str;
    if( zoneHint ) {
        zone_hint_str = zoneHint;
    }

    status = getAndConnRcatHost(rsComm, SLAVE_RCAT, zoneHint,
				&rodsServerHost);

    if (status < 0) {
       return(status);
    }

    // =-=-=-=-=-=-=-
    // handle non-eirods connections
    if( !zone_hint_str.empty() ) {
        eirods::error ret = proc_query_terms_for_non_eirods_server( zone_hint_str, genQueryInp );
        if( !ret.ok() ) {
            eirods::log( PASS( ret ) );
        }
    } 

    if (rodsServerHost->localFlag == LOCAL_HOST) {
#ifdef RODS_CAT
       status = _rsGenQuery (rsComm, genQueryInp, genQueryOut);
#else
       rodsLog(LOG_NOTICE, 
	       "rsGenQuery error. RCAT is not configured on this host");
       return (SYS_NO_RCAT_SERVER_ERR);
#endif 
    } else {
        status = rcGenQuery(rodsServerHost->conn,
			   genQueryInp, genQueryOut);
    }
    if (status < 0  && status != CAT_NO_ROWS_FOUND) {
        rodsLog (LOG_NOTICE,
		 "rsGenQuery: rcGenQuery failed, status = %d", status);
    }
    return (status);
}

#ifdef RODS_CAT
int
_rsGenQuery (rsComm_t *rsComm, genQueryInp_t *genQueryInp,
	     genQueryOut_t **genQueryOut)
{
    int status;

    static int ruleExecuted=0;
    ruleExecInfo_t rei;


    static int PrePostProcForGenQueryFlag = -2;    
    int i, argc;
    ruleExecInfo_t rei2;
    char *args[MAX_NUM_OF_ARGS_IN_ACTION];
    
    if (PrePostProcForGenQueryFlag < 0) {
      if (getenv("PREPOSTPROCFORGENQUERYFLAG") != NULL)
	PrePostProcForGenQueryFlag = 1;
      else
	PrePostProcForGenQueryFlag = 0;
    }

    memset ((char*)&rei2, 0, sizeof (ruleExecInfo_t));
    rei2.rsComm = rsComm;
    if (rsComm != NULL) {
      rei2.uoic = &rsComm->clientUser;
      rei2.uoip = &rsComm->proxyUser;
    }

    /*  printGenQI(genQueryInp);  for debug */

    *genQueryOut = (genQueryOut_t*)malloc(sizeof(genQueryOut_t));
    memset((char *)*genQueryOut, 0, sizeof(genQueryOut_t));

    if (ruleExecuted==0) {
       memset((char*)&rei,0,sizeof(rei));
       rei.rsComm = rsComm;
       if (rsComm != NULL) {
          /* Include the user info for possible use by the rule.  Note
	     that when this is called (as the agent is initializing),
	     this user info is not confirmed yet.  For password
	     authentication though, the agent will soon exit if this
	     is not valid.  But tor GSI, the user information may not
	     be present and/or may be changed when the authentication
	     completes, so it may not be safe to use this in a GSI
	     enabled environment.  This addition of user information
	     was requested by ARCS/IVEC (Sean Fleming) to avoid a
	     local patch.
          */
	  rei.uoic = &rsComm->clientUser;
	  rei.uoip = &rsComm->proxyUser;
       }

       if (getRuleEngineStatus() == UNINITIALIZED) { 
          /* Skip the call to run acAclPolicy if the Rule Engine
             hasn't been initialized yet, which happens for a couple
             initial queries made by the agent when starting up.  The
             new RE logs these types of errors and so this avoids that.
          */
          status = -1;
       }
       else 
       {
          status = applyRule ("acAclPolicy", NULL, &rei, NO_SAVE_REI);
       }
       if (status==0) {
	  ruleExecuted=1; /* No need to retry next time since it
                             succeeded.  Since this is called at
                             startup, the Rule Engine may not be
                             initialized yet, in which case the
                             default setting is fine and we should
                             retry next time. */
       }
    }

    chlGenQueryAccessControlSetup(rsComm->clientUser.userName, 
			      rsComm->clientUser.rodsZone,
            rsComm->clientAddr,
	 		      rsComm->clientUser.authInfo.authFlag,
			      -1);
    if (PrePostProcForGenQueryFlag == 1) {
      args[0] = (char *) malloc(300);
      sprintf(args[0],"%ld",(long) genQueryInp);
      argc = 1;
      i =  applyRuleArg("acPreProcForGenQuery",args,argc, &rei2, NO_SAVE_REI);
      free(args[0]);
      if (i < 0) {
	rodsLog (LOG_ERROR,
		 "rsGenQuery:acPreProcForGenQuery error,stat=%d", i);
        if (i != NO_MICROSERVICE_FOUND_ERR)
	  return i;
      }
    }
    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

    status = chlGenQuery(*genQueryInp, *genQueryOut);

    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
    if (PrePostProcForGenQueryFlag == 1) {
      args[0] = (char *) malloc(300);
      args[1] = (char *) malloc(300);
      args[2] = (char *) malloc(300);
      sprintf(args[0],"%ld",(long) genQueryInp);
      sprintf(args[1],"%ld",(long) *genQueryOut);
      sprintf(args[2],"%d",status);
      argc = 3;
      i =  applyRuleArg("acPostProcForGenQuery",args,argc, &rei2, NO_SAVE_REI);
      free(args[0]);
      free(args[1]);
      free(args[2]);
      if (i < 0) {
        rodsLog (LOG_ERROR,
                 "rsGenQuery:acPostProcForGenQuery error,stat=%d", i);
	if (i != NO_MICROSERVICE_FOUND_ERR)
	  return i;
      }
    }
    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

    if (status < 0) {
       clearGenQueryOut (*genQueryOut);
       free (*genQueryOut);
       *genQueryOut = NULL;
       if (status != CAT_NO_ROWS_FOUND) {
	  rodsLog (LOG_NOTICE, 
		   "_rsGenQuery: genQuery status = %d", status);
       }
       return (status);
    }
    return (status);
} 
#endif
