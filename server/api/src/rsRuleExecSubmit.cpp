#include "rodsConnect.h"
#include "ruleExecSubmit.h"
#include "icatHighLevelRoutines.hpp"
#include "rcMisc.h"
#include "miscServerFunct.hpp"
#include "rsRuleExecSubmit.hpp"

#include "irods_log.hpp"
#include "irods_get_full_path_for_config_file.hpp"
#include "irods_random.hpp"
#include "irods_configuration_keywords.hpp"

int
rsRuleExecSubmit( rsComm_t *rsComm, ruleExecSubmitInp_t *ruleExecSubmitInp,
                  char **ruleExecId ) {
    rodsServerHost_t *rodsServerHost;
    int status;

    *ruleExecId = nullptr;

    if ( ruleExecSubmitInp == nullptr ||
            ruleExecSubmitInp->packedReiAndArgBBuf == nullptr ||
            ruleExecSubmitInp->packedReiAndArgBBuf->len <= 0 ||
            ruleExecSubmitInp->packedReiAndArgBBuf->buf == nullptr ) {
        rodsLog( LOG_NOTICE,
                 "rsRuleExecSubmit error. NULL input" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    status = getAndConnReHost( rsComm, &rodsServerHost );
    if ( status < 0 ) {
        return status;
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
        std::string svc_role;
        irods::error ret = get_catalog_service_role(svc_role);
        if(!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }

        if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            status = _rsRuleExecSubmit( rsComm, ruleExecSubmitInp );
            if ( status >= 0 ) {
                *ruleExecId = strdup( ruleExecSubmitInp->ruleExecId );
            }
        } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
            rodsLog( LOG_NOTICE,
                     "rsRuleExecSubmit error. ICAT is not configured on this host" );
            return SYS_NO_ICAT_SERVER_ERR;
        } else {
            rodsLog(
                LOG_ERROR,
                "role not supported [%s]",
                svc_role.c_str() );
            status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
        }

    }
    else {
        if ( getValByKey( &ruleExecSubmitInp->condInput, EXEC_LOCALLY_KW ) !=
                nullptr ) {
            rodsLog( LOG_ERROR,
                     "rsRuleExecSubmit: reHost config error. reServer not running locally" );
            return SYS_CONFIG_FILE_ERR;
        }
        else {
            addKeyVal( &ruleExecSubmitInp->condInput, EXEC_LOCALLY_KW, "" );
        }
        status = rcRuleExecSubmit( rodsServerHost->conn, ruleExecSubmitInp,
                                   ruleExecId );
    }
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "rsRuleExecSubmit: rcRuleExecSubmit failed, status = %d",
                 status );
    }
    return status;
}

int
_rsRuleExecSubmit( rsComm_t *rsComm, ruleExecSubmitInp_t *ruleExecSubmitInp ) {
    int reiFd;
    int status;

    /* write the packedReiAndArgBBuf to local file */
    while ( 1 ) {
        status = getReiFilePath( ruleExecSubmitInp->reiFilePath,
                                 ruleExecSubmitInp->userName );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "rsRuleExecSubmit: getReiFilePath failed, status = %d", status );
            return status;
        }
        reiFd = open( ruleExecSubmitInp->reiFilePath, O_CREAT | O_EXCL | O_RDWR,
                      0640 );
        if ( reiFd < 0 ) {
            if ( errno == EEXIST ) {
                continue;
            }
            else {
                status = SYS_OPEN_REI_FILE_ERR - errno;
                rodsLog( LOG_ERROR,
                         "rsRuleExecSubmit: create failed for %s, status = %d",
                         ruleExecSubmitInp->reiFilePath, status );
                return status;
            }
        }
        else {
            break;
        }
    }

    status = write( reiFd, ruleExecSubmitInp->packedReiAndArgBBuf->buf,
                    ruleExecSubmitInp->packedReiAndArgBBuf->len );

    close( reiFd );

    if ( status != ruleExecSubmitInp->packedReiAndArgBBuf->len ) {
        rodsLog( LOG_ERROR,
                 "rsRuleExecSubmit: write rei error.toWrite %d, %d written",
                 ruleExecSubmitInp->packedReiAndArgBBuf->len, status );
        return SYS_COPY_LEN_ERR - errno;
    }

    /* register the request */
    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        status = chlRegRuleExec( rsComm, ruleExecSubmitInp );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "_rsRuleExecSubmit: chlRegRuleExec error. status = %d", status );
        }
        return status;
    } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        rodsLog( LOG_ERROR,
                 "_rsRuleExecSubmit error. ICAT is not configured on this host" );
        return SYS_NO_ICAT_SERVER_ERR;
    } else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        return SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }
}

int
getReiFilePath( char *reiFilePath, char *userName ) {
    char *myUserName;

    if ( reiFilePath == nullptr ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( userName == nullptr || strlen( userName ) == 0 ) {
        myUserName = DEF_REI_USER_NAME;
    }
    else {
        myUserName = userName;
    }

    std::string rei_dir;
    irods::error ret = irods::get_full_path_for_unmoved_configs(
                           PACKED_REI_DIR,
                           rei_dir );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    snprintf(
        reiFilePath,
        MAX_NAME_LEN,
        "%-s/%-s.%-s.%-u",
        rei_dir.c_str(),
        REI_FILE_NAME,
        myUserName,
        irods::getRandom<unsigned int>() );

    return 0;
}
