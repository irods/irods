////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Plug-in defining a replicating resource. This resource makes sure that all of its data is replicated to all of its children
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// =-=-=-=-=-=-=-
// irods includes
#include "msParam.h"
#include "rodsLog.h"
#include "icatHighLevelRoutines.hpp"
#include "dataObjRepl.h"

// =-=-=-=-=-=-=-
#include "irods_resource_plugin.hpp"
#include "irods_file_object.hpp"
#include "irods_collection_object.hpp"
#include "irods_string_tokenize.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_backport.hpp"
#include "irods_plugin_base.hpp"
#include "irods_stacktrace.hpp"
#include "irods_repl_types.hpp"
#include "irods_object_oper.hpp"
#include "irods_replicator.hpp"
#include "irods_create_write_replicator.hpp"
#include "irods_unlink_replicator.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_repl_rebalance.hpp"
#include "irods_kvp_string_parser.hpp"
#include "irods_random.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <list>
#include <boost/lexical_cast.hpp>

// =-=-=-=-=-=-=-
// system includes
#ifndef _WIN32
#include <sys/file.h>
#include <sys/param.h>
#endif
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/types.h>
#if defined(osx_platform)
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif
#include <fcntl.h>
#ifndef _WIN32
#include <sys/file.h>
#include <unistd.h>
#endif
#include <dirent.h>

#if defined(solaris_platform)
#include <sys/statvfs.h>
#endif
#if defined(linux_platform)
#include <sys/vfs.h>
#endif
#include <sys/stat.h>

#include <string.h>


const std::string NUM_REPL_KW( "num_repl" );
const std::string READ_KW( "read" );
const std::string READ_RANDOM_POLICY( "random" );

/// @brief Check the general parameters passed in to most plugin functions
template< typename DEST_TYPE >
irods::error replCheckParams(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    // =-=-=-=-=-=-=-
    // verify that the resc context is valid
    irods::error ret = _ctx.valid< DEST_TYPE >();
    if ( !ret.ok() ) {
        result = PASSMSG( "resource context is invalid", ret );
    }

    return result;
}

/// =-=-=-=-=-=-=-
/// @brief limit of the number of repls to operate upon during rebalance
const int DEFAULT_LIMIT = 500;

// =-=-=-=-=-=-=-
// 2. Define operations which will be called by the file*
//    calls declared in server/driver/include/fileDriver.h
// =-=-=-=-=-=-=-

// =-=-=-=-=-=-=-
// NOTE :: to access properties in the _prop_map do the
//      :: following :
//      :: double my_var = 0.0;
//      :: irods::error ret = _prop_map.get< double >( "my_key", my_var );
// =-=-=-=-=-=-=-

//////////////////////////////////////////////////////////////////////
// Utility functions

/**
 * @brief Gets the name of the child of this resource from the hierarchy
 */
irods::error replGetNextRescInHier(
    const irods::hierarchy_parser& _parser,
    irods::plugin_context& _ctx,
    irods::resource_ptr& _ret_resc ) {
    irods::error result = SUCCESS();
    irods::error ret;
    std::string this_name;
    ret = _ctx.prop_map().get<std::string>( irods::RESOURCE_NAME, this_name );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Failed to get resource name from property map.";
        result = ERROR( -1, msg.str() );
    }
    else {
        std::string child;
        ret = _parser.next( this_name, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in the hierarchy.";
            result = ERROR( -1, msg.str() );
        }
        else {
            irods::resource_child_map* cmap_ref;
            _ctx.prop_map().get< irods::resource_child_map* >(
                irods::RESC_CHILD_MAP_PROP,
                cmap_ref );
            _ret_resc = ( *cmap_ref )[child].second;
        }
    }
    return result;
}

/// @brief Returns true if the specified object is in the specified object list
bool replObjectInList(
    const object_list_t& _object_list,
    const irods::file_object_ptr _object,
    irods::object_oper& _rtn_oper ) {
    bool result = false;
    object_list_t::const_iterator it;
    for ( it = _object_list.begin(); !result && it != _object_list.end(); ++it ) {
        irods::object_oper oper = *it;
        if ( oper.object() == ( *_object.get() ) ) {
            _rtn_oper = oper;
            result = true;
        }
    }
    return result;
}

/// @brief Updates the fields in the resources properties for the object
irods::error replUpdateObjectAndOperProperties(
    irods::plugin_context& _ctx,
    const std::string& _oper ) {
    irods::error result = SUCCESS();
    irods::error ret;
    object_list_t object_list;
    // The object list is now a queue of operations and their associated objects. Their corresponding replicating operations
    // will be performed one at a time in the order in which they were put into the queue.
    irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object >( ( _ctx.fco() ) );
    ret = _ctx.prop_map().get<object_list_t>( object_list_prop, object_list );
    irods::object_oper oper;
    if ( !ret.ok() && ret.code() != KEY_NOT_FOUND ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Failed to get the object list from the resource.";
        result = PASSMSG( msg.str(), ret );
    }
    else if ( replObjectInList( object_list, file_obj, oper ) ) {
        // confirm the operations are compatible
        bool mismatched = false;
        if ( _oper == irods::CREATE_OPERATION ) {
            if ( oper.operation() != create_oper ) {
                mismatched = true;
            }
        }
        else if ( _oper == irods::WRITE_OPERATION ) {
            // write is allowed after create
            if ( oper.operation() != create_oper && oper.operation() != write_oper ) {
                mismatched = true;
            }
        } /*else if(_oper == unlink_oper) {
            if(oper.operation() != unlink_oper) {
                mismatched = true;
            }
        }*/

        result = ASSERT_ERROR( !mismatched, INVALID_OPERATION,
                               "Existing object operation: \"%s\" does not match current operation: \"%s\".",
                               oper.operation().c_str(), _oper.c_str() );
    }
    else {
        oper.object() = *( file_obj.get() );
        oper.operation() = _oper;
        object_list.push_back( oper );
        ret = _ctx.prop_map().set<object_list_t>( object_list_prop, object_list );
        result = ASSERT_PASS( ret, "Failed to set the object list property on the resource." );
    }

    if ( !result.ok() ) {
        irods::log( result );
    }

    return result;
}

irods::error get_selected_hierarchy(
    irods::plugin_context& _ctx,
    std::string& _hier_string,
    std::string& _root_resc ) {
    irods::error result = SUCCESS();
    irods::error ret;
    irods::hierarchy_parser selected_parser;
    ret = _ctx.prop_map().get<irods::hierarchy_parser>( hierarchy_prop, selected_parser );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Failed to get the parser for the selected resource hierarchy.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        ret = selected_parser.str( _hier_string );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the hierarchy string from the parser.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = selected_parser.first_resc( _root_resc );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the root resource from the parser.";
                result = PASSMSG( msg.str(), ret );
            }
        }
    }
    return result;
}

irods::error replReplicateCreateWrite(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;
    // get the list of objects that need to be replicated
    object_list_t object_list;
    ret = _ctx.prop_map().get<object_list_t>( object_list_prop, object_list );
    if ( !ret.ok() && ret.code() != KEY_NOT_FOUND ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Failed to get object list for replication.";
        result = PASSMSG( msg.str(), ret );
    }
    else if ( object_list.size() > 0 ) {
        // get the child list
        child_list_t child_list;
        ret = _ctx.prop_map().get<child_list_t>( write_child_list_prop, child_list );
        if ( ret.ok() ) {
            // get the root resource name as well as the child hierarchy string
            std::string root_resc;
            std::string child;
            ret = get_selected_hierarchy( _ctx, child, root_resc );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to determine the root resource and selected hierarchy.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                std::string name;
                ret = _ctx.prop_map().get<std::string>( irods::RESOURCE_NAME, name );
                if ( ( result = ASSERT_PASS( ret, "Could not determine resource name." ) ).ok() ) {
                    // create a create/write replicator
                    irods::create_write_replicator oper_repl( root_resc, name, child );

                    // create a replicator
                    irods::replicator replicator( &oper_repl );
                    // call replicate
                    ret = replicator.replicate( _ctx, child_list, object_list );
                    if ( !ret.ok() ) {
                        std::stringstream msg;
                        msg << __FUNCTION__;
                        msg << " - Failed to replicate the create/write operation to the siblings.";
                        result = PASSMSG( msg.str(), ret );
                    }
                    else {

                        // update the object list in the properties
                        ret = _ctx.prop_map().set<object_list_t>( object_list_prop, object_list );
                        if ( !ret.ok() ) {
                            std::stringstream msg;
                            msg << __FUNCTION__;
                            msg << " - Failed to update the object list in the properties.";
                            result = PASSMSG( msg.str(), ret );
                        }
                    }
                }
            }
        }
    }
    else {

    }
    return result;
}

irods::error replReplicateUnlink(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    // get the list of objects that need to be replicated
    object_list_t object_list;
    ret = _ctx.prop_map().get<object_list_t>( object_list_prop, object_list );
    if ( !ret.ok() && ret.code() != KEY_NOT_FOUND ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Failed to get object list for replication.";
        result = PASSMSG( msg.str(), ret );
    }
    else if ( object_list.size() > 0 ) {
        // get the child list
        child_list_t child_list;
        ret = _ctx.prop_map().get<child_list_t>( child_list_prop, child_list );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to retrieve child list from repl resource.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            // get the root resource name as well as the child hierarchy string
            std::string root_resc;
            std::string child;
            ret = get_selected_hierarchy( _ctx, child, root_resc );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to determine the root resource and selected hierarchy.";
                result = PASSMSG( msg.str(), ret );
            }
            else if ( false ) { // We no longer replicate unlink operations. Too dangerous deleting user data. Plus hopefully the
                // API handles this. - harry

                // Get the name of the current resource
                std::string current_resc;
                ret = _ctx.prop_map().get<std::string>( irods::RESOURCE_NAME, current_resc );
                if ( ( result = ASSERT_PASS( ret, "Failed to get the resource name." ) ).ok() ) {

                    // create an unlink replicator
                    irods::unlink_replicator oper_repl( child, current_resc );

                    // create a replicator
                    irods::replicator replicator( &oper_repl );

                    // call replicate
                    ret = replicator.replicate( _ctx, child_list, object_list );
                    if ( !ret.ok() ) {
                        std::stringstream msg;
                        msg << __FUNCTION__;
                        msg << " - Failed to replicate the unlink operation to the siblings.";
                        result = PASSMSG( msg.str(), ret );
                    }
                    else {

                        // update the object list in the properties
                        ret = _ctx.prop_map().set<object_list_t>( object_list_prop, object_list );
                        if ( !ret.ok() ) {
                            std::stringstream msg;
                            msg << __FUNCTION__;
                            msg << " - Failed to update the object list in the properties.";
                            result = PASSMSG( msg.str(), ret );
                        }
                    }
                }
            }
        }
    }
    return result;
}

//////////////////////////////////////////////////////////////////////
// Actual operations

// Called after a new file is registered with the ICAT
irods::error repl_file_registered(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;
    ret = replCheckParams< irods::file_object >( _ctx );
    if ( ( result = ASSERT_PASS( ret, "Error checking passed paramters." ) ).ok() ) {

        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object >( _ctx.fco() );;
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( ( result = ASSERT_PASS( ret, "Failed to get the next resource in hierarchy." ) ).ok() ) {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_REGISTERED, _ctx.fco() );
            result = ASSERT_PASS( ret, "Failed while calling child operation." );
        }
    }
    return result;
}

// Called when a file is unregistered from the ICAT
irods::error repl_file_unregistered(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;
    ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Error found checking passed parameters.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( file_obj->comm(), irods::RESOURCE_OP_UNREGISTERED, file_obj ) ;
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
        }
    }
    return result;
}

// Called when a files entry is modified in the ICAT
irods::error repl_file_modified(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;
    ret = replCheckParams< irods::file_object >( _ctx );
    if ( ( result = ASSERT_PASS( ret, "Error checking passed parameters." ) ).ok() ) {

        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( ( result = ASSERT_PASS( ret, "Failed to get the next resource in hierarchy." ) ).ok() ) {

            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_MODIFIED, _ctx.fco() );
            if ( ( result = ASSERT_PASS( ret, "Failed while calling child operation." ) ).ok() ) {

                irods::hierarchy_parser sub_parser;
                sub_parser.set_string( file_obj->in_pdmo() );
                std::string name;
                ret = _ctx.prop_map().get<std::string>( irods::RESOURCE_NAME, name );
                if ( ( result = ASSERT_PASS( ret, "Failed to get the resource name." ) ).ok() ) {
                    if ( !sub_parser.resc_in_hier( name ) ) {

                        std::string operation;
                        if ( ( ret = _ctx.prop_map().get< std::string >( operation_type_prop, operation ) ).ok() ) {
                            if ( !( ret = replUpdateObjectAndOperProperties( _ctx, operation ) ).ok() ) {
                                std::stringstream msg;
                                msg << "Failed to select an appropriate child.";
                                result = PASSMSG( msg.str(), ret );
                            }
                        }

                        ret = replReplicateCreateWrite( _ctx );
                        result = ASSERT_PASS( ret, "Failed to replicate create/write operation for object: \"%s\".",
                                              file_obj->logical_path().c_str() );
                    }
                }
            }
        }
    }
    return result;
}

// =-=-=-=-=-=-=-
// interface for POSIX create
irods::error repl_file_create(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    // get the list of objects that need to be replicated
    object_list_t object_list;
    ret = _ctx.prop_map().get<object_list_t>( object_list_prop, object_list );

    ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "repl_file_create - bad params.", ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_CREATE, _ctx.fco() );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
        }
    }
    return result;
} // repl_file_create

// =-=-=-=-=-=-=-
// interface for POSIX Open
irods::error repl_file_open(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object >( ( _ctx.fco() ) );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_OPEN, _ctx.fco() );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
        }
    }
    return result;
} // repl_file_open

// =-=-=-=-=-=-=-
// interface for POSIX Read
irods::error repl_file_read(
    irods::plugin_context& _ctx,
    void*                          _buf,
    int                            _len ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object >( ( _ctx.fco() ) );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call<void*, int>( _ctx.comm(), irods::RESOURCE_OP_READ, _ctx.fco(), _buf, _len );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                // have to return the actual code because it contains the number of bytes read
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_read

// =-=-=-=-=-=-=-
// interface for POSIX Write
irods::error repl_file_write(
    irods::plugin_context& _ctx,
    void*                          _buf,
    int                            _len ) {
    irods::error result = SUCCESS();
    irods::error ret;
    // get the list of objects that need to be replicated
    object_list_t object_list;
    ret = _ctx.prop_map().get<object_list_t>( object_list_prop, object_list );

    ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object >( ( _ctx.fco() ) );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call<void*, int>( _ctx.comm(), irods::RESOURCE_OP_WRITE, _ctx.fco(), _buf, _len );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );

            }
        }
    }
    return result;
} // repl_file_write

// =-=-=-=-=-=-=-
// interface for POSIX Close
irods::error repl_file_close(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;
    // get the list of objects that need to be replicated
    object_list_t object_list;
    ret = _ctx.prop_map().get<object_list_t>( object_list_prop, object_list );

    ret = replCheckParams< irods::file_object >( _ctx );
    if ( ( result = ASSERT_PASS( ret, "Bad params." ) ).ok() ) {

        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( ( result = ASSERT_PASS( ret, "Failed to get the next resource in hierarchy." ) ).ok() ) {

            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_CLOSE, _ctx.fco() );
            result = ASSERT_PASS( ret, "Failed while calling child operation." );
        }
    }
    return result;

} // repl_file_close

// =-=-=-=-=-=-=-
// interface for POSIX Unlink
irods::error repl_file_unlink(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::data_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::data_object_ptr data_obj = boost::dynamic_pointer_cast<irods::data_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( data_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_UNLINK, _ctx.fco() );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );

            }
        }
    }
    return result;
} // repl_file_unlink

// =-=-=-=-=-=-=-
// interface for POSIX Stat
irods::error repl_file_stat(
    irods::plugin_context& _ctx,
    struct stat*                   _statbuf ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::data_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::data_object_ptr data_obj = boost::dynamic_pointer_cast< irods::data_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( data_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call<struct stat*>( _ctx.comm(), irods::RESOURCE_OP_STAT, _ctx.fco(), _statbuf );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_stat

// =-=-=-=-=-=-=-
// interface for POSIX lseek
irods::error repl_file_lseek(
    irods::plugin_context& _ctx,
    long long                       _offset,
    int                             _whence ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call<long long, int>( _ctx.comm(), irods::RESOURCE_OP_LSEEK, _ctx.fco(), _offset, _whence );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_lseek

// =-=-=-=-=-=-=-
// interface for POSIX mkdir
irods::error repl_file_mkdir(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::collection_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::collection_object_ptr collection_obj = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( collection_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_MKDIR, _ctx.fco() );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_mkdir

// =-=-=-=-=-=-=-
// interface for POSIX mkdir
irods::error repl_file_rmdir(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::collection_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::collection_object_ptr file_obj = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_RMDIR, _ctx.fco() );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_rmdir

// =-=-=-=-=-=-=-
// interface for POSIX opendir
irods::error repl_file_opendir(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::collection_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::collection_object_ptr collection_obj = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( collection_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_OPENDIR, _ctx.fco() );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_opendir

// =-=-=-=-=-=-=-
// interface for POSIX closedir
irods::error repl_file_closedir(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::collection_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::collection_object_ptr collection_obj = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( collection_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_CLOSEDIR, _ctx.fco() );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_closedir

// =-=-=-=-=-=-=-
// interface for POSIX readdir
irods::error repl_file_readdir(
    irods::plugin_context& _ctx,
    struct rodsDirent**            _dirent_ptr ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::collection_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::collection_object_ptr collection_obj = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( collection_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call<rodsDirent**>( _ctx.comm(), irods::RESOURCE_OP_READDIR, _ctx.fco(), _dirent_ptr );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_readdir

// =-=-=-=-=-=-=-
// interface for POSIX readdir
irods::error repl_file_rename(
    irods::plugin_context& _ctx,
    const char*                    _new_file_name ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call<const char*>( _ctx.comm(), irods::RESOURCE_OP_RENAME, _ctx.fco(), _new_file_name );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_rename

// =-=-=-=-=-=-=-
// interface for POSIX truncate
irods::error repl_file_truncate(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams<irods::file_object>( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr ptr = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( ptr->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_TRUNCATE, _ctx.fco() );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_truncate

// =-=-=-=-=-=-=-
// interface to determine free space on a device given a path
irods::error repl_file_getfs_freespace(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_FREESPACE, _ctx.fco() );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_getfs_freespace

// =-=-=-=-=-=-=-
// repl_file_stage_to_cache - This routine is for testing the TEST_STAGE_FILE_TYPE.
// Just copy the file from filename to cacheFilename. optionalInfo info
// is not used.
irods::error repl_file_stage_to_cache(
    irods::plugin_context& _ctx,
    const char*                      _cache_file_name ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call< const char* >(
                      _ctx.comm(),
                      irods::RESOURCE_OP_STAGETOCACHE,
                      _ctx.fco(),
                      _cache_file_name );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
        }
    }
    return result;
} // repl_file_stage_to_cache

// =-=-=-=-=-=-=-
// repl_file_sync_to_arch - This routine is for testing the TEST_STAGE_FILE_TYPE.
// Just copy the file from cacheFilename to filename. optionalInfo info
// is not used.
irods::error repl_file_sync_to_arch(
    irods::plugin_context& _ctx,
    const char*                    _cache_file_name ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call<const char*>( _ctx.comm(), irods::RESOURCE_OP_SYNCTOARCH, _ctx.fco(), _cache_file_name );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_sync_to_arch

/// @brief Adds the current resource to the specified resource hierarchy
irods::error replAddSelfToHierarchy(
    irods::plugin_context& _ctx,
    irods::hierarchy_parser& _parser ) {
    irods::error result = SUCCESS();
    irods::error ret;
    std::string name;
    ret = _ctx.prop_map().get<std::string>( irods::RESOURCE_NAME, name );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Failed to get the resource name.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        ret = _parser.add_child( name );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to add resource to hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
    }
    return result;
}

/// @brief Loop through the children and call redirect on each one to populate the hierarchy vector
irods::error replRedirectToChildren(
    irods::plugin_context& _ctx,
    const std::string*             _operation,
    const std::string*             _curr_host,
    irods::hierarchy_parser&      _parser,
    redirect_map_t&                _redirect_map ) {
    irods::error result = SUCCESS();
    irods::error ret;
    irods::resource_child_map::iterator it;
    float out_vote;

    irods::resource_child_map* cmap_ref;
    _ctx.prop_map().get< irods::resource_child_map* >(
            irods::RESC_CHILD_MAP_PROP,
            cmap_ref );

    for ( it = cmap_ref->begin(); result.ok() && it != cmap_ref->end(); ++it ) {
        irods::hierarchy_parser parser( _parser );
        irods::resource_ptr child = it->second.second;
        ret = child->call<const std::string*, const std::string*, irods::hierarchy_parser*, float*>(
                  _ctx.comm(), irods::RESOURCE_OP_RESOLVE_RESC_HIER, _ctx.fco(), _operation, _curr_host, &parser, &out_vote );
        if ( !ret.ok() && ret.code() != CHILD_NOT_FOUND ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed calling redirect on the child \"" << it->first << "\"";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            _redirect_map.insert( std::pair<float, irods::hierarchy_parser>( out_vote, parser ) );
        }
    }
    return result;
}

/// @brief Creates a list of hierarchies to which this operation must be replicated, all children except the one on which we are
/// operating.
irods::error replCreateChildReplList(
    irods::plugin_context& _ctx,
    const redirect_map_t& _redirect_map,
    const std::string     _child_list_prop ) {
    irods::error result = SUCCESS();
    irods::error ret;

    // Check for an existing child list property. If it exists assume it is correct and do nothing.
    // This assumes that redirect always resolves to the same child. Is that ok? - hcj
    child_list_t repl_vector;
    ret = _ctx.prop_map().get<child_list_t>( _child_list_prop, repl_vector );
    if ( !ret.ok() ) {

        // loop over all of the children in the map except the first (selected) and add them to a vector
        redirect_map_t::const_iterator it = _redirect_map.begin();
        for ( ++it; it != _redirect_map.end(); ++it ) {
            // JMC - need to consider the vote here as if it is 0 the child
            //       is down and should not get a replica
            std::string hier;
            it->second.str( hier );
            if ( it->first > 0 ) {
                irods::hierarchy_parser parser = it->second;
                repl_vector.push_back( parser );
            }
        }

        // add the resulting vector as a property of the resource
        irods::error ret = _ctx.prop_map().set<child_list_t>( _child_list_prop, repl_vector );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to store the repl child list as a property.";
            result = PASSMSG( msg.str(), ret );
        }
    }
    return result;
}

/// @brief honor the read context string keyword if present
irods::error process_redirect_map_for_random_open(
    const redirect_map_t&    _redirect_map,
    irods::hierarchy_parser* _out_parser,
    float*                   _out_vote ) {

    std::vector<std::pair<float, irods::hierarchy_parser>> items;
    redirect_map_t::const_iterator itr = _redirect_map.cbegin();
    for( ; itr != _redirect_map.cend(); ++itr ) {
        if(itr->first > 0) {
            items.push_back(std::make_pair(itr->first, itr->second));
        }
    }

    size_t rand_index = irods::getRandom<size_t>() % items.size();
    std::vector<std::pair<float, irods::hierarchy_parser>>::iterator a_itr = items.begin();
    std::advance(a_itr, rand_index);
    *_out_vote   = a_itr->first;
    *_out_parser = a_itr->second;

    return SUCCESS();
} // process_redirect_map_for_random_open

/// @brief Selects a child from the vector of parsers based on host access
irods::error replSelectChild(
    irods::plugin_context&   _ctx,
    const std::string&       _operation,
    const redirect_map_t&    _redirect_map,
    const std::string&       _child_list_prop,
    const std::string&       _hierarchy_prop,
    irods::hierarchy_parser* _out_parser,
    float*                   _out_vote ) {

    (*_out_vote) = 0.0;
    if( _redirect_map.empty() ) {
        // there are no votes to consider
        return SUCCESS();
    }

    irods::error ret;
    if( irods::OPEN_OPERATION == _operation) {
        std::string read_policy;
        ret = _ctx.prop_map().get<std::string>(READ_KW, read_policy);
        if(ret.ok() && READ_RANDOM_POLICY == read_policy) {
            ret = process_redirect_map_for_random_open(
                      _redirect_map,
                      _out_parser,
                      _out_vote);
            if(!ret.ok()) {
                return PASS(ret);
            }
            return SUCCESS();
        }
    }

    irods::error result = SUCCESS();

    redirect_map_t::const_iterator it;
    it = _redirect_map.begin();
    float vote = it->first;
    irods::hierarchy_parser parser = it->second;
    *_out_parser = parser;
    *_out_vote = vote;
    if ( vote != 0.0 ) {
        ret = replCreateChildReplList( _ctx, _redirect_map, _child_list_prop );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to add unselected children to the replication list.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = _ctx.prop_map().set<irods::hierarchy_parser>( _hierarchy_prop, parser );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to add hierarchy property to resource.";
                result = PASSMSG( msg.str(), ret );
            }
        }
    }

    return result;
}

/// @brief Make sure the requested operation on the requested file object is valid
irods::error replValidOperation(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    // cast the first class object to a file object
    try {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object >( _ctx.fco() );
        // if the file object has a requested replica then fail since that circumvents the coordinating nodes management.
        if ( false && file_obj->repl_requested() >= 0 ) { // For migration we no longer have this restriction but will be added back later - harry
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Requesting replica: " << file_obj->repl_requested();
            msg << "\tCannot request specific replicas from replicating resource.";
            result = ERROR( INVALID_OPERATION, msg.str() );
        }

        else {
            // if the api commands involve replication we have to error out since managing replicas is our job
            char* in_repl = getValByKey( &file_obj->cond_input(), IN_REPL_KW );
            if ( false && in_repl != NULL ) { // For migration we no longer have this restriction but might be added later. - harry
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Using repl or trim commands on a replication resource is not allowed. ";
                msg << "Managing replicas is the job of the replication resource.";
                result = ERROR( INVALID_OPERATION, msg.str() );
            }
        }
    }
    catch ( const std::bad_cast& ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Invalid first class object.";
        result = ERROR( INVALID_FILE_OBJECT, msg.str() );
    }


    return result;
}

/// @brief Determines which child should be used for the specified operation
irods::error repl_redirect_impl(
    irods::plugin_context& _ctx,
    const std::string*              _operation,
    const std::string*              _curr_host,
    const std::string&              _child_list_prop,
    const std::string&              _hierarchy_prop,
    irods::hierarchy_parser*        _inout_parser,
    float*                          _out_vote ) {
    irods::error result = SUCCESS();
    irods::error ret;
    irods::hierarchy_parser parser = *_inout_parser;
    redirect_map_t redirect_map;

    // Make sure this is a valid repl operation.
    if ( !( ret = replValidOperation( _ctx ) ).ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Invalid operation on replicating resource.";
        result = PASSMSG( msg.str(), ret );
    }

    // add ourselves to the hierarchy parser
    else if ( !( ret = replAddSelfToHierarchy( _ctx, parser ) ).ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Failed to add ourselves to the resource hierarchy.";
        result = PASSMSG( msg.str(), ret );
    }

    // call redirect on each child with the appropriate parser
    else if ( !( ret = replRedirectToChildren( _ctx, _operation, _curr_host, parser, redirect_map ) ).ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Failed to redirect to all children.";
        result = PASSMSG( msg.str(), ret );
    }

    // foreach child parser determine the best to access based on host
    else if ( !( ret = replSelectChild( _ctx, *_operation, redirect_map, _child_list_prop, _hierarchy_prop, _inout_parser, _out_vote ) ).ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Failed to select an appropriate child.";
        result = PASSMSG( msg.str(), ret );
    }

    else if ( irods::WRITE_OPERATION  == ( *_operation ) ||
              irods::CREATE_OPERATION == ( *_operation ) ) {
        result = ASSERT_PASS( _ctx.prop_map().set< std::string >( operation_type_prop, *_operation ), "failed to set operation_type property" );
    }

    return result;

} // repl_redirect_impl

irods::error proc_child_list_for_create_policy(
        irods::plugin_context& _ctx ) {
    size_t num_repl = 0;
    irods::error ret = _ctx.prop_map().get<size_t>(NUM_REPL_KW, num_repl);
    if( !ret.ok() ) {
        return SUCCESS();
    }

    child_list_t new_list;
    if(num_repl <= 1) {
        ret = _ctx.prop_map().set<child_list_t>(write_child_list_prop, new_list);
        if(!ret.ok()) {
            return PASS(ret);
        }
        return SUCCESS();
    }

    child_list_t child_list;
    ret = _ctx.prop_map().get<child_list_t>(write_child_list_prop, child_list);
    if(!ret.ok()) {
        return PASS(ret);
    }

    // decrement num_repl by 1 to count first replica
    num_repl--;

    if( num_repl < child_list.size() ) {
        bool pick_done = false;
        std::vector<size_t> picked_indicies;
        while( !pick_done ) {
             size_t rand_index = irods::getRandom<size_t>() % child_list.size();
             if(std::find(
                 picked_indicies.begin(),
                 picked_indicies.end(),
                 rand_index) == picked_indicies.end()) {
                 new_list.push_back(child_list[rand_index]);
                 picked_indicies.push_back(rand_index);
                 if(picked_indicies.size() >= num_repl) {
                     pick_done = true;
                 }
             }
        }

        // if we have an empty new_list, simply keep the old one
        if(new_list.size() == 0) {
            return SUCCESS();
        }

        ret = _ctx.prop_map().set<child_list_t>(write_child_list_prop, new_list);
        if(!ret.ok()) {
            return PASS(ret);
        }
    }

    return SUCCESS();

} // proc_child_list_for_create_policy

irods::error repl_file_resolve_hierarchy(
    irods::plugin_context&   _ctx,
    const std::string*       _operation,
    const std::string*       _curr_host,
    irods::hierarchy_parser* _inout_parser,
    float*                   _out_vote ) {
    irods::error ret;
    if (*_operation != irods::UNLINK_OPERATION) {
        // recreate the child list for a write operation as the
        // initial voting may have resulted in a child voting 0
        // for the initial operation ( e.g. READ ).  if the repl
        // node decided to replicate the data object, a new
        // child list should be created for that operation ( WRITE )
        // issue #2789
        float vote = 0.0;
        std::string tmp_hier_prop = hierarchy_prop + "_tmp";
        std::string op = irods::CREATE_OPERATION;
        if( irods::CREATE_OPERATION != *_operation ) {
            op = irods::WRITE_OPERATION;
        }

        // NOTE:: we need a copy of the upstream parser to preserve the
        // hierarchy.  otherwise we will only generate partial hierarchies
        irods::hierarchy_parser parser = (*_inout_parser);
        ret = repl_redirect_impl(
                            _ctx,
                            &op,
                            _curr_host,
                            write_child_list_prop,
                            tmp_hier_prop,
                            &parser,
                            &vote );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        if( 0.0 == vote ) {
            std::string hier;
            parser.str( hier );
            rodsLog(
                LOG_ERROR,
                "repl_file_resolve_hierarchy - vote of 0 on create operation for [%s]", hier.c_str() );
        }

        if(irods::CREATE_OPERATION == *_operation) {
            ret = proc_child_list_for_create_policy(_ctx);
            if(!ret.ok()) {
                return PASS(ret);
            }
        }

    } // if (*_operation != irods::UNLINK_OPERATION)

    ret = repl_redirect_impl(
              _ctx,
              _operation,
              _curr_host,
              child_list_prop,
              hierarchy_prop,
              _inout_parser,
              _out_vote );
    if( !ret.ok() ) {
        return PASS( ret );
    }

    return SUCCESS();

} // repl_file_resolve_hierarchy

static
irods::error get_child_name_from_bundle(
    const std::string&   _resc_name,
    const leaf_bundle_t& _bundle,
    std::string&         _child_name ) {

    std::string hier;
    
    irods::error ret = resc_mgr.leaf_id_to_hier(
                           _bundle[0],
                           hier); 
    if(!ret.ok()) {
        return PASS(ret);
    }

    irods::hierarchy_parser parse;
    ret = parse.set_string(hier);
    if(!ret.ok()) {
        return PASS(ret);
    }

    ret = parse.next(_resc_name,_child_name);
    if(!ret.ok()) {
        return PASS(ret);
    }

    return SUCCESS();

} // get_child_name_from_bundle

// =-=-=-=-=-=-=-
// repl_file_rebalance - code which would rebalance the subtree
irods::error repl_file_rebalance(
    irods::plugin_context& _ctx ) {
    using namespace irods;

    // =-=-=-=-=-=-=-
    // forward request for rebalance to children first, then
    // rebalance ourselves.

    resource_child_map* cmap_ref;
    _ctx.prop_map().get< resource_child_map* >(
            RESC_CHILD_MAP_PROP,
            cmap_ref );

    error result = SUCCESS();
    resource_child_map::iterator itr = cmap_ref->begin();
    for ( ; itr != cmap_ref->end(); ++itr ) {
        error ret = itr->second.second->call(
                               _ctx.comm(),
                               RESOURCE_OP_REBALANCE,
                               _ctx.fco() );
        if ( !ret.ok() ) {
            log( PASS( ret ) );
            result = ret;
        }
    }

    // =-=-=-=-=-=-=-
    // if our children had errors, then we bail
    if ( !result.ok() ) {
        return PASS( result );
    }

    // =-=-=-=-=-=-=-
    // get the property 'name' of this resource
    std::string resc_name;
    error ret = _ctx.prop_map().get< std::string >(
                           RESOURCE_NAME,
                           resc_name );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // determine limit size
    int limit = DEFAULT_LIMIT;
    if ( !_ctx.rule_results().empty() ) {
        kvp_map_t kvp;
        error kvp_err = parse_kvp_string(
                                   _ctx.rule_results(),
                                   kvp );
        if ( !kvp_err.ok() ) {
            return PASS( kvp_err );
        }

        std::string limit_str = kvp[ REPL_LIMIT_KEY ];
        if ( !limit_str.empty() ) {
            try {
                limit = boost::lexical_cast<int>( limit_str );

            }
            catch ( const boost::bad_lexical_cast& ) {
                std::stringstream msg;
                msg << "failed to cast value ["
                    << limit_str
                    << "] to an integer";
                return ERROR(
                           SYS_INVALID_INPUT_PARAM,
                           msg.str() );
            }
        }

    } // if rule results not empty

    // =-=-=-=-=-=-=-
    // determine distinct root count ( may have changed )
    int status = 0;
    long long root_count = 0;
    status = chlGetDistinctDataObjCountOnResource(
                 resc_name.c_str(),
                 root_count );
    if ( status < 0 ) {
        std::stringstream msg;
        msg << "failed to get distinct cound for ["
            << resc_name
            << "]";
        return ERROR( status, msg.str().c_str() );
    }
    
    // =-=-=-=-=-=-=-
    // gather bundles of leaf ids
    std::vector<leaf_bundle_t> leaf_bundles;
    ret = resc_mgr.gather_leaf_bundles_for_resc(
              resc_name,
              leaf_bundles);
    if(!ret.ok()) {
        return PASS(ret);
    }
    
    // =-=-=-=-=-=-=-
    // iterate over the leaf bundles
    for( size_t idx = 0;
         idx < leaf_bundles.size();
         ++idx ) {
        std::string child_name;
        error get_ret = get_child_name_from_bundle(
                            resc_name,
                            leaf_bundles[idx],
                            child_name );
        if(!get_ret.ok() ) {
            irods::log(PASS(get_ret));
            continue;
        }

        dist_child_result_t results;
        error ga_ret = gather_data_objects_for_rebalance(
                                  _ctx.comm(),
                                  limit,
                                  idx,
                                  leaf_bundles,
                                  results );
        if ( !ga_ret.ok() ) {
            return PASS(ga_ret);
        }

        if( !results.empty() ) {
            error proc_ret = proc_results_for_rebalance(
                                 _ctx.comm(),
                                 resc_name,
                                 child_name,
                                 idx,
                                 leaf_bundles,
                                 results );
            if ( !proc_ret.ok() ) {
                return PASS( proc_ret );
            }
        }

    } // for c_itr

    return SUCCESS();

} // repl_file_rebalance

// Called when a files entry is modified in the ICAT
irods::error repl_file_notify(
    irods::plugin_context& _ctx,
    const std::string*               _opr ) {
    irods::error result = SUCCESS();
    if ( irods::CREATE_OPERATION == ( *_opr ) ||
            irods::WRITE_OPERATION  == ( *_opr ) ) {
        result = ASSERT_PASS( _ctx.prop_map().set< std::string >( operation_type_prop, *_opr ), "failed to set opetion_type property" );
    }

    irods::error ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object >( ( _ctx.fco() ) );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_NOTIFY, _ctx.fco(), _opr );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
        }
    }
    return result;
}

// =-=-=-=-=-=-=-
// 3. create derived class to handle unix file system resources
//    necessary to do custom parsing of the context string to place
//    any useful values into the property map for reference in later
//    operations.  semicolon is the preferred delimiter
class repl_resource : public irods::resource {

    public:
        repl_resource(
            const std::string& _inst_name,
            const std::string& _context ) :
            irods::resource(
                    _inst_name,
                    _context ) {
                irods::kvp_map_t kvp_map;
                if ( !_context.empty() ) {
                    irods::error ret = irods::parse_kvp_string(
                            _context,
                            kvp_map );
                    if ( !ret.ok() ) {
                        irods::log( PASS( ret ) );

                    }

                    if ( kvp_map.find( NUM_REPL_KW ) != kvp_map.end() ) {
                        try {
                            // need to capture as int to test for <0 first
                            int num_repl = boost::lexical_cast< int >( kvp_map[ NUM_REPL_KW ] );
                            if(num_repl <= 0) {
                                rodsLog(
                                    LOG_ERROR,
                                    "%s:%d - %s is <= 0",
                                    __FUNCTION__,
                                    __LINE__,
                                    NUM_REPL_KW.c_str());
                            }

                            properties_.set< size_t >( NUM_REPL_KW, static_cast<size_t>(num_repl) );
                        }
                        catch ( const boost::bad_lexical_cast& ) {
                            std::stringstream msg;
                            msg << "failed to cast num_repl ["
                                << kvp_map[ NUM_REPL_KW ]
                                << "]";
                            irods::log(
                                    ERROR(
                                        SYS_INVALID_INPUT_PARAM,
                                        msg.str() ) );
                        }
                    }

                    if ( kvp_map.find( READ_KW ) != kvp_map.end() ) {
                        properties_.set< std::string >( READ_KW, kvp_map[ READ_KW ] );
                    }

                } // if !empty

            } // ctor

        irods::error post_disconnect_maintenance_operation(
            irods::pdmo_type& ) {
            irods::error result = SUCCESS();
            // nothing to do
            return result;
        }

        irods::error need_post_disconnect_maintenance_operation(
            bool& _flag ) {
            irods::error result = SUCCESS();
            _flag = false;
            return result;
        }

}; // class repl_resource

// =-=-=-=-=-=-=-
// 4. create the plugin factory function which will return a dynamically
//    instantiated object of the previously defined derived resource.  use
//    the add_operation member to associate a 'call name' to the interfaces
//    defined above.  for resource plugins these call names are standardized
//    as used by the irods facing interface defined in
//    server/drivers/src/fileDriver.c
extern "C"
irods::resource* plugin_factory( const std::string& _inst_name, const std::string& _context ) {

    // =-=-=-=-=-=-=-
    // 4a. create repl_resource
    repl_resource* resc = new repl_resource( _inst_name, _context );

    // =-=-=-=-=-=-=-
    // 4b. map function names to operations.  this map will be used to load
    //     the symbols from the shared object in the delay_load stage of
    //     plugin loading.
    using namespace irods;
    using namespace std;
    resc->add_operation(
        RESOURCE_OP_CREATE,
        function<error(plugin_context&)>(
            repl_file_create ) );

    resc->add_operation(
        irods::RESOURCE_OP_OPEN,
        function<error(plugin_context&)>(
            repl_file_open ) );

    resc->add_operation<void*,int>(
        irods::RESOURCE_OP_READ,
        std::function<
            error(irods::plugin_context&,void*,int)>(
                repl_file_read ) );

    resc->add_operation<void*,int>(
        irods::RESOURCE_OP_WRITE,
        function<error(plugin_context&,void*,int)>(
            repl_file_write ) );

    resc->add_operation(
        RESOURCE_OP_CLOSE,
        function<error(plugin_context&)>(
            repl_file_close ) );

    resc->add_operation(
        irods::RESOURCE_OP_UNLINK,
        function<error(plugin_context&)>(
            repl_file_unlink ) );

    resc->add_operation<struct stat*>(
        irods::RESOURCE_OP_STAT,
        function<error(plugin_context&, struct stat*)>(
            repl_file_stat ) );

    resc->add_operation(
        irods::RESOURCE_OP_MKDIR,
        function<error(plugin_context&)>(
            repl_file_mkdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_OPENDIR,
        function<error(plugin_context&)>(
            repl_file_opendir ) );

    resc->add_operation<struct rodsDirent**>(
        irods::RESOURCE_OP_READDIR,
        function<error(plugin_context&,struct rodsDirent**)>(
            repl_file_readdir ) );

    resc->add_operation<const char*>(
        irods::RESOURCE_OP_RENAME,
        function<error(plugin_context&, const char*)>(
            repl_file_rename ) );

    resc->add_operation(
        irods::RESOURCE_OP_FREESPACE,
        function<error(plugin_context&)>(
            repl_file_getfs_freespace ) );

    resc->add_operation<long long, int>(
        irods::RESOURCE_OP_LSEEK,
        function<error(plugin_context&, long long, int)>(
            repl_file_lseek ) );

    resc->add_operation(
        irods::RESOURCE_OP_RMDIR,
        function<error(plugin_context&)>(
            repl_file_rmdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_CLOSEDIR,
        function<error(plugin_context&)>(
            repl_file_closedir ) );

    resc->add_operation<const char*>(
        irods::RESOURCE_OP_STAGETOCACHE,
        function<error(plugin_context&, const char*)>(
            repl_file_stage_to_cache ) );

    resc->add_operation<const char*>(
        irods::RESOURCE_OP_SYNCTOARCH,
        function<error(plugin_context&, const char*)>(
            repl_file_sync_to_arch ) );

    resc->add_operation(
        irods::RESOURCE_OP_REGISTERED,
        function<error(plugin_context&)>(
            repl_file_registered ) );

    resc->add_operation(
        irods::RESOURCE_OP_UNREGISTERED,
        function<error(plugin_context&)>(
            repl_file_unregistered ) );

    resc->add_operation(
        irods::RESOURCE_OP_MODIFIED,
        function<error(plugin_context&)>(
            repl_file_modified ) );

    resc->add_operation<const std::string*>(
        irods::RESOURCE_OP_NOTIFY,
        function<error(plugin_context&, const std::string*)>(
            repl_file_notify ) );

    resc->add_operation(
        irods::RESOURCE_OP_TRUNCATE,
        function<error(plugin_context&)>(
            repl_file_truncate ) );

    resc->add_operation<const std::string*, const std::string*, irods::hierarchy_parser*, float*>(
        irods::RESOURCE_OP_RESOLVE_RESC_HIER,
        function<error(plugin_context&,const std::string*, const std::string*, irods::hierarchy_parser*, float*)>(
            repl_file_resolve_hierarchy ) );

    resc->add_operation(
        irods::RESOURCE_OP_REBALANCE,
        function<error(plugin_context&)>(
            repl_file_rebalance ) );

    // =-=-=-=-=-=-=-
    // set some properties necessary for backporting to iRODS legacy code
    resc->set_property< int >( irods::RESOURCE_CHECK_PATH_PERM, 2 );//DO_CHK_PATH_PERM );
    resc->set_property< int >( irods::RESOURCE_CREATE_PATH,     1 );//CREATE_PATH );

    // =-=-=-=-=-=-=-
    // 4c. return the pointer through the generic interface of an
    //     irods::resource pointer
    return dynamic_cast<irods::resource*>( resc );

} // plugin_factory



