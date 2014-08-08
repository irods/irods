/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See generalAdmin.h for a description of this API call.*/

// =-=-=-=-=-=-=-
// irods includes
#include "generalAdmin.hpp"
#include "reGlobalsExtern.hpp"
#include "icatHighLevelRoutines.hpp"
#include "reFuncDefs.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>
#include <string>

// =-=-=-=-=-=-=-
#include "irods_children_parser.hpp"
#include "irods_string_tokenize.hpp"
#include "irods_plugin_name_generator.hpp"
#include "irods_resources_home.hpp"
#include "irods_resource_manager.hpp"
#include "irods_file_object.hpp"
extern irods::resource_manager resc_mgr;



int
rsGeneralAdmin( rsComm_t *rsComm, generalAdminInp_t *generalAdminInp ) {
    rodsServerHost_t *rodsServerHost;
    int status;

    rodsLog( LOG_DEBUG, "generalAdmin" );

    status = getAndConnRcatHost( rsComm, MASTER_RCAT, NULL, &rodsServerHost );
    if ( status < 0 ) {
        return status;
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
#ifdef RODS_CAT
        status = _rsGeneralAdmin( rsComm, generalAdminInp );
#else
        status = SYS_NO_RCAT_SERVER_ERR;
#endif
    }
    else {
        status = rcGeneralAdmin( rodsServerHost->conn,
                                 generalAdminInp );

        // =-=-=-=-=-=-=-
        // always replicate the error stack [#2184] for 'iadmin lt' reporting
        // back to the client if it had been redirected
        replErrorStack( rodsServerHost->conn->rError, &rsComm->rError );

    }
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "rsGeneralAdmin: rcGeneralAdmin error %d", status );
    }
    return status;
}

#ifdef RODS_CAT
int
_addChildToResource(
    generalAdminInp_t* _generalAdminInp,
    ruleExecInfo_t&    _rei2,
    rsComm_t*          _rsComm ) {
    int result = 0;
    rescInfo_t rescInfo;
    memset( &rescInfo, 0, sizeof( rescInfo ) );
    strncpy( rescInfo.rescName, _generalAdminInp->arg2, sizeof rescInfo.rescName );
    std::string rescChild( _generalAdminInp->arg3 );
    std::string rescContext( _generalAdminInp->arg4 );
    irods::children_parser parser;
    parser.add_child( rescChild, rescContext );
    std::string rescChildren;
    parser.str( rescChildren );
    strncpy( rescInfo.rescChildren, rescChildren.c_str(), sizeof rescInfo.rescChildren );

    rodsLog( LOG_NOTICE, "rsGeneralAdmin add child \"%s\" to resource \"%s\"", rescChildren.c_str(),
             rescInfo.rescName );

    if ( ( result = chlAddChildResc( _rsComm, &rescInfo ) ) != 0 ) {
        chlRollback( _rsComm );
    }

    return result;
}

int
_removeChildFromResource(
    generalAdminInp_t* _generalAdminInp,
    ruleExecInfo_t&    _rei2,
    rsComm_t*          _rsComm ) {
    int result = 0;
    rescInfo_t rescInfo;

    strncpy( rescInfo.rescName, _generalAdminInp->arg2, sizeof rescInfo.rescName );
    strncpy( rescInfo.rescChildren, _generalAdminInp->arg3, sizeof rescInfo.rescChildren );

    rodsLog( LOG_NOTICE, "rsGeneralAdmin remove child \"%s\" from resource \"%s\"", rescInfo.rescChildren,
             rescInfo.rescName );

    if ( ( result = chlDelChildResc( _rsComm, &rescInfo ) ) != 0 ) {
        chlRollback( _rsComm );
    }

    return result;
}

/* extracted out the functionality for adding a resource to the grid - hcj */
int
_addResource(
    generalAdminInp_t* _generalAdminInp,
    ruleExecInfo_t&    _rei2,
    rsComm_t*          _rsComm ) {
    int result = 0;
    rescInfo_t rescInfo;
    bzero( &rescInfo, sizeof( rescInfo ) );

    static const unsigned int argc = 7;
    char *args[argc];

    // =-=-=-=-=-=-=-
    // capture all the parameters
    std::string resc_name( _generalAdminInp->arg2 );
    std::string resc_type( _generalAdminInp->arg3 );
    std::string resc_host_path( _generalAdminInp->arg4 );
    std::string resc_ctx( _generalAdminInp->arg5 );

    // =-=-=-=-=-=-=-
    // if it is not empty, parse out the host:path otherwise
    // fill in with the EMPTY placeholders
    if ( !resc_host_path.empty() ) {
        // =-=-=-=-=-=-=-
        // separate the location:/vault/path pair
        std::vector< std::string > tok;
        irods::string_tokenize( resc_host_path, ":", tok );

        // =-=-=-=-=-=-=-
        // if we have exactly 2 tokens, things are going well
        if ( 2 == tok.size() ) {
            // =-=-=-=-=-=-=-
            // location is index 0, path is index 1
            strncpy( rescInfo.rescLoc,       tok[0].c_str(), sizeof rescInfo.rescLoc );
            strncpy( rescInfo.rescVaultPath, tok[1].c_str(), sizeof rescInfo.rescVaultPath );
        }

    }
    else {
        strncpy( rescInfo.rescLoc,       irods::EMPTY_RESC_HOST.c_str(), sizeof rescInfo.rescLoc );
        strncpy( rescInfo.rescVaultPath, irods::EMPTY_RESC_PATH.c_str(), sizeof rescInfo.rescVaultPath );

    }

    // =-=-=-=-=-=-=-
    // pull values out of api call args into rescInfo structure
    strncpy( rescInfo.rescName,     _generalAdminInp->arg2, sizeof rescInfo.rescName );
    strncpy( rescInfo.rescType,     _generalAdminInp->arg3, sizeof rescInfo.rescType );
    strncpy( rescInfo.rescContext,  _generalAdminInp->arg5, sizeof rescInfo.rescContext );
    strncpy( rescInfo.rescClass,    "cache",                sizeof rescInfo.rescClass );
    strncpy( rescInfo.zoneName,     _generalAdminInp->arg6, sizeof rescInfo.zoneName );
    strncpy( rescInfo.rescChildren, "", 1 );
    strncpy( rescInfo.rescParent,   "", 1 );

    // =-=-=-=-=-=-=-
    // RAJA ADDED June 1 2009 for pre-post processing rule hooks
    args[0] = rescInfo.rescName;
    args[1] = rescInfo.rescType;
    args[2] = rescInfo.rescClass;
    args[3] = rescInfo.rescLoc;
    args[4] = rescInfo.rescVaultPath;
    args[5] = rescInfo.rescContext;
    args[6] = rescInfo.zoneName;

    // =-=-=-=-=-=-=-
    // Check that there is a plugin matching the resource type
    irods::plugin_name_generator name_gen;
    if ( !name_gen.exists( rescInfo.rescType, irods::RESOURCES_HOME ) ) {
        rodsLog(
            LOG_DEBUG,
            "No plugin exists to provide resource [%s] of type [%s]",
            rescInfo.rescName,
            rescInfo.rescType );
    }

    // =-=-=-=-=-=-=-
    // apply preproc policy enforcement point for creating a resource, handle errors
    if ( ( result =  applyRuleArg( "acPreProcForCreateResource", args, argc, &_rei2, NO_SAVE_REI ) ) < 0 ) {
        if ( _rei2.status < 0 ) {
            result = _rei2.status;
        }
        rodsLog( LOG_ERROR, "rsGeneralAdmin:acPreProcForCreateResource error for %s,stat=%d",
                 rescInfo.rescName, result );
        /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
    }

    // =-=-=-=-=-=-=-
    // register resource with the metadata catalog, roll back on an error
    else if ( ( result = chlRegResc( _rsComm, &rescInfo ) ) != 0 ) {
        chlRollback( _rsComm );
    }

    // =-=-=-=-=-=-=-
    // apply postproc policy enforcement point for creating a resource, handle errors
    // ( RAJA ADDED June 1 2009 for pre-post processing rule hooks )
    else if ( ( result =  applyRuleArg( "acPostProcForCreateResource", args, argc, &_rei2, NO_SAVE_REI ) ) < 0 ) {
        if ( _rei2.status < 0 ) {
            result = _rei2.status;
        }
        rodsLog( LOG_ERROR, "rsGeneralAdmin:acPostProcForCreateResource error for %s,stat=%d",
                 rescInfo.rescName, result );
    }
    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

    return result;
}

int
_listRescTypes( rsComm_t* _rsComm ) {
    int result = 0;
    irods::plugin_name_generator name_gen;
    irods::plugin_name_generator::plugin_list_t plugin_list;
    irods::error ret = name_gen.list_plugins( irods::RESOURCES_HOME, plugin_list );
    if ( ret.ok() ) {
        std::stringstream msg;
        for ( irods::plugin_name_generator::plugin_list_t::iterator it = plugin_list.begin();
                result == 0 && it != plugin_list.end(); ++it ) {
            msg << *it << std::endl;
        }
        if ( result == 0 ) {
            addRErrorMsg( &_rsComm->rError, STDOUT_STATUS, msg.str().c_str() );
        }
    }
    else {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Failed to generate the list of resource plugins.";
        irods::error res = PASSMSG( msg.str(), ret );
        irods::log( res );
        result = res.code();
    }
    return result;
}

int
_rsGeneralAdmin( rsComm_t *rsComm, generalAdminInp_t *generalAdminInp ) {
    int status;
    collInfo_t collInfo;
    rescInfo_t rescInfo;
    ruleExecInfo_t rei;
    char *args[MAX_NUM_OF_ARGS_IN_ACTION];
    int i, argc;
    ruleExecInfo_t rei2;

    memset( ( char* )&rei2, 0, sizeof( ruleExecInfo_t ) );
    rei2.rsComm = rsComm;
    if ( rsComm != NULL ) {
        rei2.uoic = &rsComm->clientUser;
        rei2.uoip = &rsComm->proxyUser;
    }


    rodsLog( LOG_DEBUG,
             "_rsGeneralAdmin arg0=%s",
             generalAdminInp->arg0 );

    if ( strcmp( generalAdminInp->arg0, "pvacuum" ) == 0 ) {
        char *args[2];
        char argStr[128];    /* argument string */
        memset( ( char* )&rei, 0, sizeof( rei ) );
        rei.rsComm = rsComm;
        rei.uoic = &rsComm->clientUser;
        rei.uoip = &rsComm->proxyUser;
        rstrcpy( argStr, "", sizeof argStr );
        if ( atoi( generalAdminInp->arg1 ) > 0 ) {
            snprintf( argStr, sizeof argStr, "<ET>%s</ET>", generalAdminInp->arg1 );
        }
        if ( atoi( generalAdminInp->arg2 ) > 0 ) {
            strncat( argStr, "<EF>", 5 );
            strncat( argStr, generalAdminInp->arg2, 100 );
            strncat( argStr, "</EF>", 6 );
        }
        args[0] = argStr;
        status = applyRuleArg( "acVacuum", args, 1, &rei, SAVE_REI );
        return status;
    }

    if ( strcmp( generalAdminInp->arg0, "add" ) == 0 ) {
        if ( strcmp( generalAdminInp->arg1, "user" ) == 0 ) {
            /* run the acCreateUser rule */
            char *args[2];
            memset( ( char* )&rei, 0, sizeof( rei ) );
            rei.rsComm = rsComm;
            userInfo_t userInfo;
            memset( &userInfo, 0, sizeof( userInfo ) );
            strncpy( userInfo.userName, generalAdminInp->arg2,
                     sizeof( userInfo.userName ) );
            if ( userInfo.userName[sizeof( userInfo.userName ) - 1] ) {
                return SYS_INVALID_INPUT_PARAM;
            }
            strncpy( userInfo.userType, generalAdminInp->arg3,
                     sizeof( userInfo.userType ) );
            if ( userInfo.userType[sizeof( userInfo.userType ) - 1] ) {
                return SYS_INVALID_INPUT_PARAM;
            }
            strncpy( userInfo.rodsZone, generalAdminInp->arg4,
                     sizeof( userInfo.rodsZone ) );
            if ( userInfo.rodsZone[sizeof( userInfo.rodsZone ) - 1] ) {
                return SYS_INVALID_INPUT_PARAM;
            }
            strncpy( userInfo.authInfo.authStr, generalAdminInp->arg5,
                     sizeof( userInfo.authInfo.authStr ) );
            if ( userInfo.authInfo.authStr[sizeof( userInfo.authInfo.authStr ) - 1] ) {
                return SYS_INVALID_INPUT_PARAM;
            }
            rei.uoio = &userInfo;
            rei.uoic = &rsComm->clientUser;
            rei.uoip = &rsComm->proxyUser;
            status = applyRuleArg( "acCreateUser", args, 0, &rei, SAVE_REI );
            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "dir" ) == 0 ) {
            memset( ( char* )&collInfo, 0, sizeof( collInfo ) );
            strncpy( collInfo.collName, generalAdminInp->arg2,
                     sizeof( collInfo.collName ) );
            if ( collInfo.collName[sizeof( collInfo.collName ) - 1] ) {
                return SYS_INVALID_INPUT_PARAM;
            }
            if ( strlen( generalAdminInp->arg3 ) > 0 ) {
                strncpy( collInfo.collOwnerName, generalAdminInp->arg3,
                         sizeof( collInfo.collOwnerName ) );
                if ( collInfo.collOwnerName[sizeof( collInfo.collOwnerName ) - 1] ) {
                    return SYS_INVALID_INPUT_PARAM;
                }
                status = chlRegCollByAdmin( rsComm, &collInfo );
                if ( status == 0 ) {
                    if ( !chlCommit( rsComm ) ) {
                        // JMC - ERROR?
                    }
                }
            }
            else {
                status = chlRegColl( rsComm, &collInfo );
            }
            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "zone" ) == 0 ) {
            status = chlRegZone( rsComm, generalAdminInp->arg2,
                                 generalAdminInp->arg3,
                                 generalAdminInp->arg4,
                                 generalAdminInp->arg5 );
            if ( status == 0 ) {
                if ( strcmp( generalAdminInp->arg3, "remote" ) == 0 ) {
                    memset( ( char* )&collInfo, 0, sizeof( collInfo ) );
                    strncpy( collInfo.collName, "/", sizeof( collInfo.collName ) );
                    strncat( collInfo.collName, generalAdminInp->arg2,
                             sizeof( collInfo.collName ) - strlen( collInfo.collName ) );
                    if ( collInfo.collName[sizeof( collInfo.collName ) - 1] ) {
                        return SYS_INVALID_INPUT_PARAM;
                    }
                    strncpy( collInfo.collOwnerName, rsComm->proxyUser.userName,
                             sizeof( collInfo.collOwnerName ) );
                    if ( collInfo.collOwnerName[sizeof( collInfo.collOwnerName ) - 1] ) {
                        return SYS_INVALID_INPUT_PARAM;
                    }
                    status = chlRegCollByAdmin( rsComm, &collInfo );
                    if ( status == 0 ) {
                        chlCommit( rsComm );
                    }
                }
            }
            return status;
        } // add user

        // =-=-=-=-=-=-=-
        // add a new resource to the data grid
        if ( strcmp( generalAdminInp->arg1, "resource" ) == 0 ) {
            return _addResource( generalAdminInp, rei2, rsComm );
        } // if create resource

        /* add a child resource to the specified parent resource */
        if ( strcmp( generalAdminInp->arg1, "childtoresc" ) == 0 ) {
            return _addChildToResource( generalAdminInp, rei2, rsComm );
        }

        if ( strcmp( generalAdminInp->arg1, "token" ) == 0 ) {
            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
            args[0] = generalAdminInp->arg2;
            args[1] = generalAdminInp->arg3;
            args[2] = generalAdminInp->arg4;
            args[3] = generalAdminInp->arg5;
            args[4] = generalAdminInp->arg6;
            args[5] = generalAdminInp->arg7;
            argc = 6;
            i =  applyRuleArg( "acPreProcForCreateToken", args, argc, &rei2, NO_SAVE_REI );
            if ( i < 0 ) {
                if ( rei2.status < 0 ) {
                    i = rei2.status;
                }
                rodsLog( LOG_ERROR,
                         "rsGeneralAdmin:acPreProcForCreateToken error for %s.%s=%s,stat=%d",
                         args[0], args[1], args[2], i );
                return i;
            }
            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

            status = chlRegToken( rsComm, generalAdminInp->arg2,
                                  generalAdminInp->arg3,
                                  generalAdminInp->arg4,
                                  generalAdminInp->arg5,
                                  generalAdminInp->arg6,
                                  generalAdminInp->arg7 );
            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
            if ( status == 0 ) {
                i =  applyRuleArg( "acPostProcForCreateToken", args, argc, &rei2, NO_SAVE_REI );
                if ( i < 0 ) {
                    if ( rei2.status < 0 ) {
                        i = rei2.status;
                    }
                    rodsLog( LOG_ERROR,
                             "rsGeneralAdmin:acPostProcForCreateToken error for %s.%s=%s,stat=%d",
                             args[0], args[1], args[2], i );
                    return i;
                }
            }
            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            return status;
        } // token

        if ( strcmp( generalAdminInp->arg1, "specificQuery" ) == 0 ) {
            status = chlAddSpecificQuery( rsComm, generalAdminInp->arg2,
                                          generalAdminInp->arg3 );
            return status;
        }

    } // add

    if ( strcmp( generalAdminInp->arg0, "modify" ) == 0 ) {
        if ( strcmp( generalAdminInp->arg1, "user" ) == 0 ) {
            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
            args[0] = generalAdminInp->arg2; /* username */
            args[1] = generalAdminInp->arg3; /* option */
            /* Since the obfuscated password might contain commas, single or
               double quotes, etc, it's hard to escape for processing (often
               causing a seg fault), so for now just pass in a dummy string.
               It is also unlikely the microservice actually needs the obfuscated
               password. */
            args[2] = "obfuscatedPw";
            argc = 3;
            i =  applyRuleArg( "acPreProcForModifyUser", args, argc, &rei2, NO_SAVE_REI );
            if ( i < 0 ) {
                if ( rei2.status < 0 ) {
                    i = rei2.status;
                }
                rodsLog( LOG_ERROR,
                         "rsGeneralAdmin:acPreProcForModifyUser error for %s and option %s,stat=%d",
                         args[0], args[1], i );
                return i;
            }
            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

            status = chlModUser( rsComm, generalAdminInp->arg2,
                                 generalAdminInp->arg3, generalAdminInp->arg4 );

            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
            if ( status == 0 ) {
                i =  applyRuleArg( "acPostProcForModifyUser", args, argc, &rei2, NO_SAVE_REI );
                if ( i < 0 ) {
                    if ( rei2.status < 0 ) {
                        i = rei2.status;
                    }
                    rodsLog( LOG_ERROR,
                             "rsGeneralAdmin:acPostProcForModifyUser error for %s and option %s,stat=%d",
                             args[0], args[1], i );
                    return i;
                }
            }
            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "group" ) == 0 ) {
            userInfo_t ui;
            memset( &ui, 0, sizeof( userInfo_t ) );
            rei2.uoio = &ui;
            rstrcpy( ui.userName, generalAdminInp->arg4, NAME_LEN );
            rstrcpy( ui.rodsZone, generalAdminInp->arg5, NAME_LEN );
            args[0] = generalAdminInp->arg2; /* groupname */
            args[1] = generalAdminInp->arg3; /* option */
            args[2] = generalAdminInp->arg4; /* username */
            args[3] = generalAdminInp->arg5; /* zonename */
            argc = 4;
            i =  applyRuleArg( "acPreProcForModifyUserGroup", args, argc, &rei2, NO_SAVE_REI );
            if ( i < 0 ) {
                if ( rei2.status < 0 ) {
                    i = rei2.status;
                }
                rodsLog( LOG_ERROR,
                         "rsGeneralAdmin:acPreProcForModifyUserGroup error for %s and option %s,stat=%d",
                         args[0], args[1], i );
                return i;
            }
            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

            status = chlModGroup( rsComm, generalAdminInp->arg2,
                                  generalAdminInp->arg3, generalAdminInp->arg4,
                                  generalAdminInp->arg5 );
            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
            if ( status == 0 ) {
                i =  applyRuleArg( "acPostProcForModifyUserGroup", args, argc, &rei2, NO_SAVE_REI );
                if ( i < 0 ) {
                    if ( rei2.status < 0 ) {
                        i = rei2.status;
                    }
                    rodsLog( LOG_ERROR,
                             "rsGeneralAdmin:acPostProcForModifyUserGroup error for %s and option %s,stat=%d",
                             args[0], args[1], i );
                    return i;
                }
            }
            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "zone" ) == 0 ) {
            status = chlModZone( rsComm, generalAdminInp->arg2,
                                 generalAdminInp->arg3, generalAdminInp->arg4 );
            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            if ( status == 0 &&
                    strcmp( generalAdminInp->arg3, "name" ) == 0 ) {
                char oldName[MAX_NAME_LEN];
                char newName[MAX_NAME_LEN];
                strncpy( oldName, "/", sizeof( oldName ) );
                strncat( oldName, generalAdminInp->arg2, sizeof( oldName ) - strlen( oldName ) );
                if ( oldName[sizeof( oldName ) - 1] ) {
                    return SYS_INVALID_INPUT_PARAM;
                }
                strncpy( newName, generalAdminInp->arg4, sizeof( newName ) );
                if ( newName[sizeof( newName ) - 1] ) {
                    return SYS_INVALID_INPUT_PARAM;
                }
                status = chlRenameColl( rsComm, oldName, newName );
                if ( status == 0 ) {
                    chlCommit( rsComm );
                }
            }
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "zonecollacl" ) == 0 ) {
            status = chlModZoneCollAcl( rsComm, generalAdminInp->arg2,
                                        generalAdminInp->arg3, generalAdminInp->arg4 );
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "localzonename" ) == 0 ) {
            /* run the acRenameLocalZone rule */
            char *args[2];
            memset( ( char* )&rei, 0, sizeof( rei ) );
            rei.rsComm = rsComm;
            memset( ( char* )&rei, 0, sizeof( rei ) );
            rei.rsComm = rsComm;
            rei.uoic = &rsComm->clientUser;
            rei.uoip = &rsComm->proxyUser;
            args[0] = generalAdminInp->arg2;
            args[1] = generalAdminInp->arg3;
            status = applyRuleArg( "acRenameLocalZone", args, 2, &rei,
                                   NO_SAVE_REI );
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "resourcedatapaths" ) == 0 ) {
            status = chlModRescDataPaths( rsComm, generalAdminInp->arg2,
                                          generalAdminInp->arg3, generalAdminInp->arg4,
                                          generalAdminInp->arg5 );

            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "resource" ) == 0 ) {

            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
            args[0] = generalAdminInp->arg2; /* rescname */
            args[1] = generalAdminInp->arg3; /* option */
            args[2] = generalAdminInp->arg4; /* newvalue */
            argc = 3;
            i =  applyRuleArg( "acPreProcForModifyResource", args, argc, &rei2, NO_SAVE_REI );
            if ( i < 0 ) {
                if ( rei2.status < 0 ) {
                    i = rei2.status;
                }
                rodsLog( LOG_ERROR,
                         "rsGeneralAdmin:acPreProcForModifyResource error for %s and option %s,stat=%d",
                         args[0], args[1], i );
                return i;
            }
            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
            // =-=-=-=-=-=-=-
            // addition of 'rebalance' as an option to iadmin modresc.  not in icat code
            // due to dependency on resc_mgr, etc.
            if ( 0 == strcmp( args[1], "rebalance" ) ) {
                status = 0;

                // =-=-=-=-=-=-=-
                // resolve the plugin we wish to rebalance
                irods::resource_ptr resc;
                irods::error ret = resc_mgr.resolve( args[0], resc );
                if ( !ret.ok() ) {
                    irods::log( PASSMSG( "failed to resolve resource", ret ) );
                    status = -1;
                }
                else {
                    // =-=-=-=-=-=-=-
                    // call the rebalance operation on the resource
                    irods::file_object_ptr obj( new irods::file_object() );
                    ret = resc->call( rsComm, irods::RESOURCE_OP_REBALANCE, obj );
                    if ( !ret.ok() ) {
                        irods::log( PASSMSG( "failed to rebalance resource", ret ) );
                        status = -1;

                    }

                }

            }
            else {
                status = chlModResc(
                             rsComm,
                             generalAdminInp->arg2,
                             generalAdminInp->arg3,
                             generalAdminInp->arg4 );

            }

            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
            if ( status == 0 ) {
                i =  applyRuleArg( "acPostProcForModifyResource", args, argc, &rei2, NO_SAVE_REI );
                if ( i < 0 ) {
                    if ( rei2.status < 0 ) {
                        i = rei2.status;
                    }
                    rodsLog( LOG_ERROR,
                             "rsGeneralAdmin:acPostProcForModifyResource error for %s and option %s,stat=%d",
                             args[0], args[1], i );
                    return i;
                }
            }
            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            return status;
        }
    }
    if ( strcmp( generalAdminInp->arg0, "rm" ) == 0 ) {
        if ( strcmp( generalAdminInp->arg1, "user" ) == 0 ) {
            /* run the acDeleteUser rule */
            char *args[2];
            memset( ( char* )&rei, 0, sizeof( rei ) );
            rei.rsComm = rsComm;
            userInfo_t userInfo;
            memset( &userInfo, 0, sizeof( userInfo ) );
            strncpy( userInfo.userName, generalAdminInp->arg2,
                     sizeof( userInfo.userName ) );
            if ( userInfo.userName[sizeof( userInfo.userName ) - 1] ) {
                return SYS_INVALID_INPUT_PARAM;
            }
            strncpy( userInfo.rodsZone, generalAdminInp->arg3,
                     sizeof( userInfo.rodsZone ) );
            if ( userInfo.rodsZone[sizeof( userInfo.rodsZone ) - 1] ) {
                return SYS_INVALID_INPUT_PARAM;
            }
            userInfo_t userInfoRei = userInfo;
            char userName[NAME_LEN];
            char zoneName[NAME_LEN];
            status = parseUserName( userInfo.userName, userName, zoneName );
            if ( status != 0 ) {
                chlRollback( rsComm );
                return status;
            }
            if ( strcmp( zoneName, "" ) != 0 ) {
                strncpy( userInfoRei.rodsZone, zoneName, NAME_LEN );
                // =-=-=-=-=-=-=-
                // JMC :: while correct, much of the code assumes that the user name
                //        has the zone name appended.  this will need to be part of
                //        a full audit of the use of userName and zoneName
                // strncpy( userInfoRei.userName, userName, NAME_LEN );
            }

            rei.uoio = &userInfoRei;
            rei.uoic = &rsComm->clientUser;
            rei.uoip = &rsComm->proxyUser;
            status = applyRuleArg( "acDeleteUser", args, 0, &rei, SAVE_REI );
            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "dir" ) == 0 ) {
            memset( ( char* )&collInfo, 0, sizeof( collInfo ) );
            strncpy( collInfo.collName, generalAdminInp->arg2,
                     sizeof( collInfo.collName ) );
            if ( collInfo.collName[sizeof( collInfo.collName ) - 1] ) {
                return SYS_INVALID_INPUT_PARAM;
            }
            status = chlDelColl( rsComm, &collInfo );
            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "resource" ) == 0 ) {

            // =-=-=-=-=-=-=-
            // JMC 04.11.2012 :: add a dry run option to idamin rmresc
            //                :: basically run chlDelResc then run a rollback immediately after
            if ( strcmp( generalAdminInp->arg3, "--dryrun" ) == 0 ) {
                strncpy( rescInfo.rescName,  generalAdminInp->arg2, sizeof rescInfo.rescName );
                if ( rescInfo.rescName[sizeof( rescInfo.rescName ) - 1] ) {
                    return SYS_INVALID_INPUT_PARAM;
                }

                rodsLog( LOG_STATUS, "Executing a dryrun of removal of resource [%s]", generalAdminInp->arg2 );

                status = chlDelResc( rsComm, &rescInfo, 1 );
                if ( 0 == status ) {
                    rodsLog( LOG_STATUS, "DRYRUN REMOVING RESOURCE [%s] :: SUCCESS", generalAdminInp->arg2 );
                }
                else {
                    rodsLog( LOG_STATUS, "DRYRUN REMOVING RESOURCE [%s] :: FAILURE", generalAdminInp->arg2 );
                }

                return status;
            } // if dryrun
            // =-=-=-=-=-=-=-

            strncpy( rescInfo.rescName,  generalAdminInp->arg2,
                     sizeof( rescInfo.rescName ) );
            if ( rescInfo.rescName[sizeof( rescInfo.rescName ) - 1] ) {
                return SYS_INVALID_INPUT_PARAM;
            }

            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
            args[0] = rescInfo.rescName;
            argc = 1;
            i =  applyRuleArg( "acPreProcForDeleteResource", args, argc, &rei2, NO_SAVE_REI );
            if ( i < 0 ) {
                if ( rei2.status < 0 ) {
                    i = rei2.status;
                }
                rodsLog( LOG_ERROR,
                         "rsGeneralAdmin:acPreProcForDeleteResource error for %s,stat=%d",
                         rescInfo.rescName, i );
                return i;
            }
            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

            status = chlDelResc( rsComm, &rescInfo );
            if ( status == 0 ) {
                /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
                i =  applyRuleArg( "acPostProcForDeleteResource", args, argc, &rei2, NO_SAVE_REI );
                if ( i < 0 ) {
                    if ( rei2.status < 0 ) {
                        i = rei2.status;
                    }
                    rodsLog( LOG_ERROR,
                             "rsGeneralAdmin:acPostProcForDeleteResource error for %s,stat=%d",
                             rescInfo.rescName, i );
                    return i;
                }
            }
            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            return status;
        }

        /* remove a child resource from the specified parent resource */
        if ( strcmp( generalAdminInp->arg1, "childfromresc" ) == 0 ) {
            return _removeChildFromResource( generalAdminInp, rei2, rsComm );
        }

        if ( strcmp( generalAdminInp->arg1, "zone" ) == 0 ) {
            status = chlDelZone( rsComm, generalAdminInp->arg2 );
            if ( status == 0 ) {
                memset( ( char* )&collInfo, 0, sizeof( collInfo ) );
                strncpy( collInfo.collName, "/", sizeof collInfo.collName );
                strncat( collInfo.collName, generalAdminInp->arg2,
                         sizeof( collInfo.collName ) - strlen( collInfo.collName ) );
                if ( collInfo.collName[sizeof( collInfo.collName ) - 1] ) {
                    return SYS_INVALID_INPUT_PARAM;
                }
                status = chlDelCollByAdmin( rsComm, &collInfo );
            }
            if ( status == 0 ) {
                status = chlCommit( rsComm );
            }
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "token" ) == 0 ) {

            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
            args[0] = generalAdminInp->arg2;
            args[1] = generalAdminInp->arg3;
            argc = 2;
            i =  applyRuleArg( "acPreProcForDeleteToken", args, argc, &rei2, NO_SAVE_REI );
            if ( i < 0 ) {
                if ( rei2.status < 0 ) {
                    i = rei2.status;
                }
                rodsLog( LOG_ERROR,
                         "rsGeneralAdmin:acPreProcForDeleteToken error for %s.%s,stat=%d",
                         args[0], args[1], i );
                return i;
            }
            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

            status = chlDelToken( rsComm, generalAdminInp->arg2,
                                  generalAdminInp->arg3 );

            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
            if ( status == 0 ) {
                i =  applyRuleArg( "acPostProcForDeleteToken", args, argc, &rei2, NO_SAVE_REI );
                if ( i < 0 ) {
                    if ( rei2.status < 0 ) {
                        i = rei2.status;
                    }
                    rodsLog( LOG_ERROR,
                             "rsGeneralAdmin:acPostProcForDeleteToken error for %s.%s,stat=%d",
                             args[0], args[1], i );
                    return i;
                }
            }
            /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "unusedAVUs" ) == 0 ) {
            status = chlDelUnusedAVUs( rsComm );
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "specificQuery" ) == 0 ) {
            status = chlDelSpecificQuery( rsComm, generalAdminInp->arg2 );
            return status;
        }
    }
    if ( strcmp( generalAdminInp->arg0, "calculate-usage" ) == 0 ) {
        status = chlCalcUsageAndQuota( rsComm );
        return status;
    }
    if ( strcmp( generalAdminInp->arg0, "set-quota" ) == 0 ) {
        status = chlSetQuota( rsComm,
                              generalAdminInp->arg1,
                              generalAdminInp->arg2,
                              generalAdminInp->arg3,
                              generalAdminInp->arg4 );

        return status;
    }

    if ( strcmp( generalAdminInp->arg0, "lt" ) == 0 ) {
        status = CAT_INVALID_ARGUMENT;
        if ( strcmp( generalAdminInp->arg1, "resc_type" ) == 0 ) {
            status = _listRescTypes( rsComm );
        }
        return status;
    }

    return CAT_INVALID_ARGUMENT;
}
#endif
