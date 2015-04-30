////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Plug-in defining a replicating resource. This resource makes sure that all of its data is replicated to all of its children
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// =-=-=-=-=-=-=-
// irods includes
#include "msParam.hpp"
#include "reGlobalsExtern.hpp"
#include "rodsLog.h"
#include "icatHighLevelRoutines.hpp"
#include "dataObjRepl.hpp"

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

/// @brief Check the general parameters passed in to most plugin functions
template< typename DEST_TYPE >
irods::error replCheckParams(
    irods::resource_plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    // =-=-=-=-=-=-=-
    // verify that the resc context is valid
    irods::error ret = _ctx.valid< DEST_TYPE >();
    if ( !ret.ok() ) {
        result = PASSMSG( "resource context is invalid", ret );
    }

    return result;
}

extern "C" {
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
        irods::resource_plugin_context& _ctx,
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
                _ret_resc = ( _ctx.child_map() )[child].second;
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
        irods::resource_plugin_context& _ctx,
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
        irods::resource_plugin_context& _ctx,
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
        irods::resource_plugin_context& _ctx ) {
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
        irods::resource_plugin_context& _ctx ) {
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
    irods::error replFileRegistered(
        irods::resource_plugin_context& _ctx ) {
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
    irods::error replFileUnregistered(
        irods::resource_plugin_context& _ctx ) {
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
    irods::error replFileModified(
        irods::resource_plugin_context& _ctx ) {
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
    irods::error replFileCreate(
        irods::resource_plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        irods::error ret;

        // get the list of objects that need to be replicated
        object_list_t object_list;
        ret = _ctx.prop_map().get<object_list_t>( object_list_prop, object_list );

        ret = replCheckParams< irods::file_object >( _ctx );
        if ( !ret.ok() ) {
            result = PASSMSG( "replFileCreatePlugin - bad params.", ret );
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
    } // replFileCreate

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    irods::error replFileOpen(
        irods::resource_plugin_context& _ctx ) {
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
    } // replFileOpen

    // =-=-=-=-=-=-=-
    // interface for POSIX Read
    irods::error replFileRead(
        irods::resource_plugin_context& _ctx,
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
    } // replFileRead

    // =-=-=-=-=-=-=-
    // interface for POSIX Write
    irods::error replFileWrite(
        irods::resource_plugin_context& _ctx,
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
    } // replFileWrite

    // =-=-=-=-=-=-=-
    // interface for POSIX Close
    irods::error replFileClose(
        irods::resource_plugin_context& _ctx ) {
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

    } // replFileClose

    // =-=-=-=-=-=-=-
    // interface for POSIX Unlink
    irods::error replFileUnlink(
        irods::resource_plugin_context& _ctx ) {
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
    } // replFileUnlink

    // =-=-=-=-=-=-=-
    // interface for POSIX Stat
    irods::error replFileStat(
        irods::resource_plugin_context& _ctx,
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
    } // replFileStat

    // =-=-=-=-=-=-=-
    // interface for POSIX lseek
    irods::error replFileLseek(
        irods::resource_plugin_context& _ctx,
        size_t                         _offset,
        int                            _whence ) {
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
                ret = child->call<size_t, int>( _ctx.comm(), irods::RESOURCE_OP_LSEEK, _ctx.fco(), _offset, _whence );
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
    } // replFileLseek

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    irods::error replFileMkdir(
        irods::resource_plugin_context& _ctx ) {
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
    } // replFileMkdir

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    irods::error replFileRmdir(
        irods::resource_plugin_context& _ctx ) {
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
    } // replFileRmdir

    // =-=-=-=-=-=-=-
    // interface for POSIX opendir
    irods::error replFileOpendir(
        irods::resource_plugin_context& _ctx ) {
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
    } // replFileOpendir

    // =-=-=-=-=-=-=-
    // interface for POSIX closedir
    irods::error replFileClosedir(
        irods::resource_plugin_context& _ctx ) {
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
    } // replFileClosedir

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    irods::error replFileReaddir(
        irods::resource_plugin_context& _ctx,
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
    } // replFileReaddir

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    irods::error replFileRename(
        irods::resource_plugin_context& _ctx,
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
    } // replFileRename

    // =-=-=-=-=-=-=-
    // interface for POSIX truncate
    irods::error replFileTruncate(
        irods::resource_plugin_context& _ctx ) {
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
    } // replFileTruncate

    // =-=-=-=-=-=-=-
    // interface to determine free space on a device given a path
    irods::error replFileGetFsFreeSpace(
        irods::resource_plugin_context& _ctx ) {
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
    } // replFileGetFsFreeSpace

    // =-=-=-=-=-=-=-
    // replStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from filename to cacheFilename. optionalInfo info
    // is not used.
    irods::error replStageToCache(
        irods::resource_plugin_context& _ctx,
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
    } // replStageToCache

    // =-=-=-=-=-=-=-
    // passthruSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from cacheFilename to filename. optionalInfo info
    // is not used.
    irods::error replSyncToArch(
        irods::resource_plugin_context& _ctx,
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
    } // replSyncToArch

    /// @brief Adds the current resource to the specified resource hierarchy
    irods::error replAddSelfToHierarchy(
        irods::resource_plugin_context& _ctx,
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
        irods::resource_plugin_context& _ctx,
        const std::string*             _operation,
        const std::string*             _curr_host,
        irods::hierarchy_parser&      _parser,
        redirect_map_t&                _redirect_map ) {
        irods::error result = SUCCESS();
        irods::error ret;
        irods::resource_child_map::iterator it;
        float out_vote;
        for ( it = _ctx.child_map().begin(); result.ok() && it != _ctx.child_map().end(); ++it ) {
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
        irods::resource_plugin_context& _ctx,
        const redirect_map_t& _redirect_map ) {
        irods::error result = SUCCESS();
        irods::error ret;

        // Check for an existing child list property. If it exists assume it is correct and do nothing.
        // This assumes that redirect always resolves to the same child. Is that ok? - hcj
        child_list_t repl_vector;
        ret = _ctx.prop_map().get<child_list_t>( child_list_prop, repl_vector );
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
            irods::error ret = _ctx.prop_map().set<child_list_t>( child_list_prop, repl_vector );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to store the repl child list as a property.";
                result = PASSMSG( msg.str(), ret );
            }
        }
        return result;
    }

    /// @brief Selects a child from the vector of parsers based on host access
    irods::error replSelectChild(
        irods::resource_plugin_context& _ctx,
        const redirect_map_t& _redirect_map,
        irods::hierarchy_parser* _out_parser,
        float* _out_vote ) {
        irods::error result = SUCCESS();
        irods::error ret;

        redirect_map_t::const_iterator it;
        it = _redirect_map.begin();
        float vote = it->first;
        irods::hierarchy_parser parser = it->second;
        *_out_parser = parser;
        *_out_vote = vote;
        if ( vote != 0.0 ) {
            ret = replCreateChildReplList( _ctx, _redirect_map );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to add unselected children to the replication list.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                ret = _ctx.prop_map().set<irods::hierarchy_parser>( hierarchy_prop, parser );
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
        irods::resource_plugin_context& _ctx ) {
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
    irods::error replRedirect(
        irods::resource_plugin_context& _ctx,
        const std::string*             _operation,
        const std::string*             _curr_host,
        irods::hierarchy_parser*      _inout_parser,
        float*                         _out_vote ) {
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
        else if ( !( ret = replSelectChild( _ctx, redirect_map, _inout_parser, _out_vote ) ).ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to select an appropriate child.";
            result = PASSMSG( msg.str(), ret );
        }

        else if ( irods::WRITE_OPERATION  == ( *_operation ) ||
                  irods::CREATE_OPERATION == ( *_operation ) ) {
            result = ASSERT_PASS( _ctx.prop_map().set< std::string >( operation_type_prop, *_operation ), "failed to set opetion_type property" );
        }

        return result;
    }

    // =-=-=-=-=-=-=-
    // replRebalance - code which would rebalance the subtree
    irods::error replRebalance(
        irods::resource_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // forward request for rebalance to children first, then
        // rebalance ourselves.
        irods::error result = SUCCESS();
        irods::resource_child_map::iterator itr = _ctx.child_map().begin();
        for ( ; itr != _ctx.child_map().end(); ++itr ) {
            irods::error ret = itr->second.second->call(
                                   _ctx.comm(),
                                   irods::RESOURCE_OP_REBALANCE,
                                   _ctx.fco() );
            if ( !ret.ok() ) {
                irods::log( PASS( ret ) );
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
        irods::error ret = _ctx.prop_map().get< std::string >(
                               irods::RESOURCE_NAME,
                               resc_name );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // determine limit size
        int limit = DEFAULT_LIMIT;
        if ( !_ctx.rule_results().empty() ) {
            irods::kvp_map_t kvp;
            irods::error kvp_err = irods::parse_kvp_string(
                                       _ctx.rule_results(),
                                       kvp );
            if ( !kvp_err.ok() ) {
                return PASS( kvp_err );
            }

            std::string limit_str = kvp[ irods::REPL_LIMIT_KEY ];
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
        // determine distinct root count
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
        // iterate over the children, if one does not have a matching
        // distinct count then we need to rebalance
        irods::resource_child_map::iterator c_itr;
        for ( c_itr  = _ctx.child_map().begin();
                c_itr != _ctx.child_map().end();
                ++c_itr ) {
            // =-=-=-=-=-=-=-
            // get list of distinct missing data ids or dirty repls, iterate
            // until query fails - no more repls necessary for child
            dist_child_result_t results( 1 );
            while ( !results.empty() ) {
                irods::error ga_ret = irods::gather_data_objects_for_rebalance(
                                          _ctx.comm(),
                                          resc_name,
                                          c_itr->first,
                                          limit,
                                          results );
                if ( ga_ret.ok() ) {
                    if ( !results.empty() ) {
                        // =-=-=-=-=-=-=-
                        // if the results are not empty call our processing function
                        irods::error proc_ret = irods::proc_results_for_rebalance(
                                                    _ctx.comm(),  // comm ptr
                                                    resc_name,    // parent resc name
                                                    c_itr->first, // child resc name
                                                    results );    // result set
                        if ( !proc_ret.ok() ) {
                            return PASS( proc_ret );
                        }

                    } // if results

                }
                else {
                    return PASS( ga_ret );

                }

            } // while

        } // for c_itr

        return SUCCESS();

    } // replRebalance

    // Called when a files entry is modified in the ICAT
    irods::error replFileNotify(
        irods::resource_plugin_context& _ctx,
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
                irods::resource( _inst_name, _context ) {
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
    irods::resource* plugin_factory( const std::string& _inst_name, const std::string& _context ) {

        // =-=-=-=-=-=-=-
        // 4a. create repl_resource
        repl_resource* resc = new repl_resource( _inst_name, _context );

        // =-=-=-=-=-=-=-
        // 4b. map function names to operations.  this map will be used to load
        //     the symbols from the shared object in the delay_load stage of
        //     plugin loading.
        resc->add_operation( irods::RESOURCE_OP_CREATE,            "replFileCreate" );
        resc->add_operation( irods::RESOURCE_OP_OPEN,              "replFileOpen" );
        resc->add_operation( irods::RESOURCE_OP_READ,              "replFileRead" );
        resc->add_operation( irods::RESOURCE_OP_WRITE,             "replFileWrite" );
        resc->add_operation( irods::RESOURCE_OP_CLOSE,             "replFileClose" );
        resc->add_operation( irods::RESOURCE_OP_UNLINK,            "replFileUnlink" );
        resc->add_operation( irods::RESOURCE_OP_STAT,              "replFileStat" );
        resc->add_operation( irods::RESOURCE_OP_MKDIR,             "replFileMkdir" );
        resc->add_operation( irods::RESOURCE_OP_OPENDIR,           "replFileOpendir" );
        resc->add_operation( irods::RESOURCE_OP_READDIR,           "replFileReaddir" );
        resc->add_operation( irods::RESOURCE_OP_RENAME,            "replFileRename" );
        resc->add_operation( irods::RESOURCE_OP_TRUNCATE,          "replFileTruncate" );
        resc->add_operation( irods::RESOURCE_OP_FREESPACE,         "replFileGetFsFreeSpace" );
        resc->add_operation( irods::RESOURCE_OP_LSEEK,             "replFileLseek" );
        resc->add_operation( irods::RESOURCE_OP_RMDIR,             "replFileRmdir" );
        resc->add_operation( irods::RESOURCE_OP_CLOSEDIR,          "replFileClosedir" );
        resc->add_operation( irods::RESOURCE_OP_STAGETOCACHE,      "replStageToCache" );
        resc->add_operation( irods::RESOURCE_OP_SYNCTOARCH,        "replSyncToArch" );
        resc->add_operation( irods::RESOURCE_OP_RESOLVE_RESC_HIER, "replRedirect" );
        resc->add_operation( irods::RESOURCE_OP_REGISTERED,        "replFileRegistered" );
        resc->add_operation( irods::RESOURCE_OP_UNREGISTERED,      "replFileUnregistered" );
        resc->add_operation( irods::RESOURCE_OP_MODIFIED,          "replFileModified" );
        resc->add_operation( irods::RESOURCE_OP_REBALANCE,         "replRebalance" );
        resc->add_operation( irods::RESOURCE_OP_NOTIFY,            "replFileNotify" );

        // =-=-=-=-=-=-=-
        // set some properties necessary for backporting to iRODS legacy code
        resc->set_property< int >( irods::RESOURCE_CHECK_PATH_PERM, 2 );//DO_CHK_PATH_PERM );
        resc->set_property< int >( irods::RESOURCE_CREATE_PATH,     1 );//CREATE_PATH );

        // =-=-=-=-=-=-=-
        // 4c. return the pointer through the generic interface of an
        //     irods::resource pointer
        return dynamic_cast<irods::resource*>( resc );

    } // plugin_factory

}; // extern "C"



