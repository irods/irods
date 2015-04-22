/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See generalAdmin.h for a description of this API call.*/

// =-=-=-=-=-=-=-
// irods includes
#include "generalAdmin.hpp"
#include "rodsConnect.h"
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
#include "irods_resource_constants.hpp"
extern irods::resource_manager resc_mgr;



int
rsGeneralAdmin( rsComm_t *rsComm, generalAdminInp_t *generalAdminInp ) {
    rodsServerHost_t *rodsServerHost;
    int status;

    rodsLog( LOG_DEBUG, "generalAdmin" );

    status = getAndConnRcatHost( rsComm, MASTER_RCAT, ( const char* )NULL, &rodsServerHost );
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
    rsComm_t*          _rsComm ) {

    int result = 0;
    std::map<std::string, std::string> resc_input;

    if ( strlen( _generalAdminInp->arg2 ) >= NAME_LEN ) {	// resource name
        return SYS_INVALID_INPUT_PARAM;
    }

    resc_input[irods::RESOURCE_NAME] = _generalAdminInp->arg2;

    std::string rescChild( _generalAdminInp->arg3 );
    std::string rescContext( _generalAdminInp->arg4 );
    irods::children_parser parser;
    parser.add_child( rescChild, rescContext );
    std::string rescChildren;
    parser.str( rescChildren );
    if ( rescChildren.length() >= MAX_PATH_ALLOWED ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    resc_input[irods::RESOURCE_CHILDREN] = rescChildren;

    rodsLog( LOG_NOTICE, "rsGeneralAdmin add child \"%s\" to resource \"%s\"", rescChildren.c_str(),
             resc_input[irods::RESOURCE_NAME].c_str() );

    if ( ( result = chlAddChildResc( _rsComm, resc_input ) ) != 0 ) {
        chlRollback( _rsComm );
    }

    return result;
}

int
_removeChildFromResource(
    generalAdminInp_t* _generalAdminInp,
    rsComm_t*          _rsComm ) {

    int result = 0;
    std::map<std::string, std::string> resc_input;

    if ( strlen( _generalAdminInp->arg2 ) >= NAME_LEN ) {	// resource name
        return SYS_INVALID_INPUT_PARAM;
    }
    resc_input[irods::RESOURCE_NAME] = _generalAdminInp->arg2;

    if ( strlen( _generalAdminInp->arg3 ) >= MAX_PATH_ALLOWED ) {
        return SYS_INVALID_INPUT_PARAM;
    }
    resc_input[irods::RESOURCE_CHILDREN] = _generalAdminInp->arg3;

    rodsLog( LOG_NOTICE, "rsGeneralAdmin remove child \"%s\" from resource \"%s\"", resc_input[irods::RESOURCE_CHILDREN].c_str(),
             resc_input[irods::RESOURCE_NAME].c_str() );

    if ( ( result = chlDelChildResc( _rsComm, resc_input ) ) != 0 ) {
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
    static const unsigned int argc = 7;
    const char *args[argc];
    std::map<std::string, std::string> resc_input;

    // =-=-=-=-=-=-=-
    // Legacy checks
    if ( strlen( _generalAdminInp->arg2 ) >= NAME_LEN ) {	// resource name
        return SYS_INVALID_INPUT_PARAM;
    }

    if ( strlen( _generalAdminInp->arg3 ) >= NAME_LEN ) {	// resource type
        return SYS_INVALID_INPUT_PARAM;
    }

    if ( strlen( _generalAdminInp->arg5 ) >= MAX_PATH_ALLOWED ) {	// resource context
        return SYS_INVALID_INPUT_PARAM;
    }

    if ( strlen( _generalAdminInp->arg6 ) >= NAME_LEN ) {	// resource zone
        return SYS_INVALID_INPUT_PARAM;
    }

    // =-=-=-=-=-=-=-
    // capture all the parameters
    resc_input[irods::RESOURCE_NAME] = _generalAdminInp->arg2;
    resc_input[irods::RESOURCE_TYPE] = _generalAdminInp->arg3;
    resc_input[irods::RESOURCE_PATH] = _generalAdminInp->arg4;
    resc_input[irods::RESOURCE_CONTEXT] = _generalAdminInp->arg5;
    resc_input[irods::RESOURCE_ZONE] = _generalAdminInp->arg6;
    resc_input[irods::RESOURCE_CLASS] = irods::RESOURCE_CLASS_CACHE;
    resc_input[irods::RESOURCE_CHILDREN] = "";
    resc_input[irods::RESOURCE_PARENT] = "";

    // =-=-=-=-=-=-=-
    // if it is not empty, parse out the host:path otherwise
    // fill in with the EMPTY placeholders
    if ( !resc_input[irods::RESOURCE_PATH].empty() ) {
        // =-=-=-=-=-=-=-
        // separate the location:/vault/path pair
        std::vector< std::string > tok;
        irods::string_tokenize( resc_input[irods::RESOURCE_PATH], ":", tok );

        // =-=-=-=-=-=-=-
        // if we have exactly 2 tokens, things are going well
        if ( 2 == tok.size() ) {
            // =-=-=-=-=-=-=-
            // location is index 0, path is index 1
            if ( strlen( tok[0].c_str() ) >= NAME_LEN ) {
                return SYS_INVALID_INPUT_PARAM;
            }
            resc_input[irods::RESOURCE_LOCATION] = tok[0];

            if ( strlen( tok[1].c_str() ) >= MAX_NAME_LEN ) {
                return SYS_INVALID_INPUT_PARAM;
            }
            resc_input[irods::RESOURCE_PATH] = tok[1];
        }

    }
    else {
        resc_input[irods::RESOURCE_LOCATION] = irods::EMPTY_RESC_HOST;
        resc_input[irods::RESOURCE_PATH] = irods::EMPTY_RESC_PATH;
    }

    // =-=-=-=-=-=-=-
    args[0] = resc_input[irods::RESOURCE_NAME].c_str();
    args[1] = resc_input[irods::RESOURCE_TYPE].c_str();
    args[2] = resc_input[irods::RESOURCE_CLASS].c_str();
    args[3] = resc_input[irods::RESOURCE_LOCATION].c_str();
    args[4] = resc_input[irods::RESOURCE_PATH].c_str();
    args[5] = resc_input[irods::RESOURCE_CONTEXT].c_str();
    args[6] = resc_input[irods::RESOURCE_ZONE].c_str();

    // =-=-=-=-=-=-=-
    // Check that there is a plugin matching the resource type
    irods::plugin_name_generator name_gen;
    if ( !name_gen.exists( resc_input[irods::RESOURCE_TYPE], irods::RESOURCES_HOME ) ) {
        rodsLog(
            LOG_DEBUG,
            "No plugin exists to provide resource [%s] of type [%s]",
            resc_input[irods::RESOURCE_NAME].c_str(),
            resc_input[irods::RESOURCE_TYPE].c_str() );
    }

    // =-=-=-=-=-=-=-
    // apply preproc policy enforcement point for creating a resource, handle errors
    if ( ( result =  applyRuleArg( "acPreProcForCreateResource", args, argc, &_rei2, NO_SAVE_REI ) ) < 0 ) {
        if ( _rei2.status < 0 ) {
            result = _rei2.status;
        }
        rodsLog( LOG_ERROR, "rsGeneralAdmin:acPreProcForCreateResource error for %s,stat=%d",
                 resc_input[irods::RESOURCE_NAME].c_str(), result );
    }

    // =-=-=-=-=-=-=-
    // register resource with the metadata catalog, roll back on an error
    else if ( ( result = chlRegResc( _rsComm, resc_input ) ) != 0 ) {
        chlRollback( _rsComm );
    }

    // =-=-=-=-=-=-=-
    // apply postproc policy enforcement point for creating a resource, handle errors
    else if ( ( result =  applyRuleArg( "acPostProcForCreateResource", args, argc, &_rei2, NO_SAVE_REI ) ) < 0 ) {
        if ( _rei2.status < 0 ) {
            result = _rei2.status;
        }
        rodsLog( LOG_ERROR, "rsGeneralAdmin:acPostProcForCreateResource error for %s,stat=%d",
                 resc_input[irods::RESOURCE_NAME].c_str(), result );
    }

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
    ruleExecInfo_t rei;
    const char *args[MAX_NUM_OF_ARGS_IN_ACTION];
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

    if ( strcmp( generalAdminInp->arg0, "add" ) == 0 ) {
        if ( strcmp( generalAdminInp->arg1, "user" ) == 0 ) {
            /* run the acCreateUser rule */
            memset( ( char* )&rei, 0, sizeof( rei ) );
            rei.rsComm = rsComm;
            userInfo_t userInfo;
            memset( &userInfo, 0, sizeof( userInfo ) );
            snprintf( userInfo.userName, sizeof( userInfo.userName ), "%s", generalAdminInp->arg2 );
            snprintf( userInfo.userType, sizeof( userInfo.userType ), "%s", generalAdminInp->arg3 );
            snprintf( userInfo.rodsZone, sizeof( userInfo.rodsZone ), "%s", generalAdminInp->arg4 );
            snprintf( userInfo.authInfo.authStr, sizeof( userInfo.authInfo.authStr ), "%s", generalAdminInp->arg5 );
            rei.uoio = &userInfo;
            rei.uoic = &rsComm->clientUser;
            rei.uoip = &rsComm->proxyUser;
            status = applyRuleArg( "acCreateUser", NULL, 0, &rei, SAVE_REI );
            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "dir" ) == 0 ) {
            memset( ( char* )&collInfo, 0, sizeof( collInfo ) );
            snprintf( collInfo.collName, sizeof( collInfo.collName ), "%s", generalAdminInp->arg2 );
            if ( strlen( generalAdminInp->arg3 ) > 0 ) {
                snprintf( collInfo.collOwnerName, sizeof( collInfo.collOwnerName ), "%s", generalAdminInp->arg3 );
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
                    snprintf( collInfo.collName, sizeof( collInfo.collName ), "/%s", generalAdminInp->arg2 );
                    snprintf( collInfo.collOwnerName, sizeof( collInfo.collOwnerName ), "%s", rsComm->proxyUser.userName );
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
            return _addChildToResource( generalAdminInp, rsComm );
        }

        if ( strcmp( generalAdminInp->arg1, "token" ) == 0 ) {
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

            status = chlRegToken( rsComm, generalAdminInp->arg2,
                                  generalAdminInp->arg3,
                                  generalAdminInp->arg4,
                                  generalAdminInp->arg5,
                                  generalAdminInp->arg6,
                                  generalAdminInp->arg7 );
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

            status = chlModUser( rsComm, generalAdminInp->arg2,
                                 generalAdminInp->arg3, generalAdminInp->arg4 );

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

            status = chlModGroup( rsComm, generalAdminInp->arg2,
                                  generalAdminInp->arg3, generalAdminInp->arg4,
                                  generalAdminInp->arg5 );
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
                snprintf( oldName, sizeof( oldName ), "/%s", generalAdminInp->arg2 );
                snprintf( newName, sizeof( newName ), "%s", generalAdminInp->arg4 );
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
            const char *args[2];
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
            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            return status;
        }
    }
    if ( strcmp( generalAdminInp->arg0, "rm" ) == 0 ) {
        if ( strcmp( generalAdminInp->arg1, "user" ) == 0 ) {
            /* run the acDeleteUser rule */
            const char *args[2];
            memset( ( char* )&rei, 0, sizeof( rei ) );
            rei.rsComm = rsComm;
            userInfo_t userInfo;
            memset( &userInfo, 0, sizeof( userInfo ) );
            snprintf( userInfo.userName, sizeof( userInfo.userName ), "%s", generalAdminInp->arg2 );
            snprintf( userInfo.rodsZone, sizeof( userInfo.rodsZone ), "%s", generalAdminInp->arg3 );
            userInfo_t userInfoRei = userInfo;
            char userName[NAME_LEN];
            char zoneName[NAME_LEN];
            status = parseUserName( userInfo.userName, userName, zoneName );
            if ( status != 0 ) {
                chlRollback( rsComm );
                return status;
            }
            if ( strcmp( zoneName, "" ) != 0 ) {
                snprintf( userInfoRei.rodsZone, sizeof( userInfoRei.rodsZone ), "%s", zoneName );
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
            snprintf( collInfo.collName, sizeof( collInfo.collName ), "%s", generalAdminInp->arg2 );
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

            std::string resc_name = "";

            // =-=-=-=-=-=-=-
            // JMC 04.11.2012 :: add a dry run option to idamin rmresc
            //                :: basically run chlDelResc then run a rollback immediately after
            if ( strcmp( generalAdminInp->arg3, "--dryrun" ) == 0 ) {

                if ( strlen( generalAdminInp->arg2 ) >= NAME_LEN ) {	// resource name
                    return SYS_INVALID_INPUT_PARAM;
                }

                resc_name = generalAdminInp->arg2;

                rodsLog( LOG_STATUS, "Executing a dryrun of removal of resource [%s]", generalAdminInp->arg2 );

                status = chlDelResc( rsComm, resc_name, 1 );
                if ( 0 == status ) {
                    rodsLog( LOG_STATUS, "DRYRUN REMOVING RESOURCE [%s] :: SUCCESS", generalAdminInp->arg2 );
                }
                else {
                    rodsLog( LOG_STATUS, "DRYRUN REMOVING RESOURCE [%s] :: FAILURE", generalAdminInp->arg2 );
                }

                return status;
            } // if dryrun
            // =-=-=-=-=-=-=-

            if ( strlen( generalAdminInp->arg2 ) >= NAME_LEN ) {	// resource name
                return SYS_INVALID_INPUT_PARAM;
            }

            resc_name = generalAdminInp->arg2;

            args[0] = resc_name.c_str();
            argc = 1;
            i =  applyRuleArg( "acPreProcForDeleteResource", args, argc, &rei2, NO_SAVE_REI );
            if ( i < 0 ) {
                if ( rei2.status < 0 ) {
                    i = rei2.status;
                }
                rodsLog( LOG_ERROR,
                         "rsGeneralAdmin:acPreProcForDeleteResource error for %s,stat=%d",
                         resc_name.c_str(), i );
                return i;
            }

            status = chlDelResc( rsComm, resc_name );
            if ( status == 0 ) {
                i =  applyRuleArg( "acPostProcForDeleteResource", args, argc, &rei2, NO_SAVE_REI );
                if ( i < 0 ) {
                    if ( rei2.status < 0 ) {
                        i = rei2.status;
                    }
                    rodsLog( LOG_ERROR,
                             "rsGeneralAdmin:acPostProcForDeleteResource error for %s,stat=%d",
                             resc_name.c_str(), i );
                    return i;
                }
            }

            if ( status != 0 ) {
                chlRollback( rsComm );
            }
            return status;
        }

        /* remove a child resource from the specified parent resource */
        if ( strcmp( generalAdminInp->arg1, "childfromresc" ) == 0 ) {
            return _removeChildFromResource( generalAdminInp, rsComm );
        }

        if ( strcmp( generalAdminInp->arg1, "zone" ) == 0 ) {
            status = chlDelZone( rsComm, generalAdminInp->arg2 );
            if ( status == 0 ) {
                memset( ( char* )&collInfo, 0, sizeof( collInfo ) );
                snprintf( collInfo.collName, sizeof( collInfo.collName ), "/%s", generalAdminInp->arg2 );
                status = chlDelCollByAdmin( rsComm, &collInfo );
            }
            if ( status == 0 ) {
                status = chlCommit( rsComm );
            }
            return status;
        }
        if ( strcmp( generalAdminInp->arg1, "token" ) == 0 ) {

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

            status = chlDelToken( rsComm, generalAdminInp->arg2,
                                  generalAdminInp->arg3 );

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
